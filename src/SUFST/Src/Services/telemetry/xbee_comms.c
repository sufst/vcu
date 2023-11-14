#include <tx_api.h>
#include "rtcan.h"
#include "xbee_comms.h"
#include "usart.h"
#include "error_handler.h"

/* Include transmit frame API layer */
#include "xbee/wpan.h"
#include "xbee/device.h"
#include "wpan/aps.h"
#include "xbee/delivery_status.h"


#define XBEE_COMMS_THREAD_PRIORITY             10
#define XBEE_COMMS_THREAD_STACK_SIZE           1024
#define XBEE_COMMS_THREAD_PREEMPTION_THRESHOLD 10                

static void xbee_comms_entry(ULONG input);
static int transmit_status_handler( xbee_dev_t *xbee, const void FAR *frame, uint16_t length, void FAR *context);

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
  {XBEE_FRAME_TRANSMIT_STATUS, 0, transmit_status_handler, NULL}, //Fill in 0x8B Handler
  XBEE_FRAME_TABLE_END
};

int transparent_rx( const wpan_envelope_t FAR *envelope, void FAR *context)
{
   return 0;
}

// must be sorted by cluster ID
static const wpan_cluster_table_entry_t digi_data_clusters[] =
{
   // transparent serial goes here (cluster 0x0011)
   { DIGI_CLUST_SERIAL, transparent_rx, NULL,
      WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL },

   // handle join notifications (cluster 0x0095) when ATAO is not 0
   XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY,

   WPAN_CLUST_ENTRY_LIST_END
};

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
static wpan_ep_state_t zdo_ep_state = { 0 };

static const wpan_endpoint_table_entry_t sample_endpoints[] = {
   ZDO_ENDPOINT(zdo_ep_state),

   // Endpoint/cluster for transparent serial and OTA command cluster
   {  WPAN_ENDPOINT_DIGI_DATA,      // endpoint
      WPAN_PROFILE_DIGI,            // profile ID
      NULL,                         // endpoint handler
      NULL,                         // ep_state
      0x0000,                       // device ID
      0x00,                         // version
      digi_data_clusters            // clusters
   },

   { WPAN_ENDPOINT_END_OF_LIST }
};

/**
 * @brief 		Creates the Xbee Comms thread
 *
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 *
 * @return		See ThreadX return codes
 */
UINT xbee_comms_init(xbee_comms_context_t* xbee_comms_ptr, error_handler_context_t* error_handler_context, TX_QUEUE* rx_queue, TX_BYTE_POOL* stack_pool_ptr)
{

    xbee_comms_ptr->rx_queue = rx_queue;
    xbee_comms_ptr->error_handler = error_handler_context;

    VOID* thread_stack_ptr = NULL;
    int xbee_status;
    //@todo Implement queue
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                XBEE_COMMS_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&xbee_comms_ptr->thread,
                               "Xbee Communication Thread",
                               xbee_comms_entry,
                               xbee_comms_ptr,
                               thread_stack_ptr,
                               XBEE_COMMS_THREAD_STACK_SIZE,
                               XBEE_COMMS_THREAD_PRIORITY,
                               XBEE_COMMS_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_AUTO_START);
    }

    if(tx_status == TX_SUCCESS)
    {
        /* Init xbee_serial_t struct */
        xbee_comms_ptr->xbee_serial.baudrate = 115200;
        xbee_comms_ptr->xbee_serial.huart = &huart4;

        xbee_status = xbee_dev_init (&xbee_comms_ptr->xbee_dev, &xbee_comms_ptr->xbee_serial, NULL, NULL);
    }
    else 
    {
        critical_error(&xbee_comms_ptr->thread, XBEE_COMMS_INIT_ERROR, xbee_comms_ptr->error_handler);
    }

    if(xbee_status == 0)
    {
        xbee_status = xbee_wpan_init(&xbee_comms_ptr->xbee_dev, sample_endpoints);
    }
    else 
    {
        critical_error(&xbee_comms_ptr->thread, XBEE_COMMS_INIT_ERROR, xbee_comms_ptr->error_handler);
    }
    
    if(xbee_status != 0)
    {
        critical_error(&xbee_comms_ptr->thread, XBEE_COMMS_INIT_ERROR, xbee_comms_ptr->error_handler);
    }

    //xbee_disc_add_node_id_handler() @todo Discovery?
    return tx_status;
}

static void xbee_comms_entry(ULONG input)
{
    xbee_comms_context_t* xbee_comms_ptr = (xbee_comms_context_t*) input;
    pdu_t* rd_pdu_ptr ;

    while(1)
    {
        int status;

        status = tx_queue_receive(xbee_comms_ptr->rx_queue, &rd_pdu_ptr , TX_WAIT_FOREVER);
        if(status!= TX_SUCCESS)
        {
            critical_error(&xbee_comms_ptr->thread, XBEE_COMMS_QUEUE_ERROR, xbee_comms_ptr->error_handler);
            return;
        }
 
        //Create envelope
        wpan_envelope_t transmit_envelope;

        wpan_envelope_create(&transmit_envelope, &xbee_comms_ptr->xbee_dev.wpan_dev, WPAN_IEEE_ADDR_BROADCAST, WPAN_NET_ADDR_UNDEFINED); 

        transmit_envelope.payload = rd_pdu_ptr ;
        transmit_envelope.length = sizeof(*rd_pdu_ptr );
        //Use an explicit transmit frame (type 0x11) with WPAN_ENDPOINT_DIGI_DATA as the source and
        /// destination endpoint, DIGI_CLUST_SERIAL as the cluster ID and WPAN_PROFILE_DIGI as the profile ID.

        //Send frame
        status = xbee_transparent_serial(&transmit_envelope);
        if(status != 0)
        {
            /* @todo Do non-critical error handling here */
        }

        //Check for newly received frames (Transmit Status)

        //wpan_tick(&xbee_dev.wpan_dev); // @todo Do we need this?
        // @todo Currently based on blocking UART function (see xbee_serial_stm32). Should it be non-blocking interrupt based? 
        // This step may be skipped if not counting stats (initially).
        xbee_dev_tick(&xbee_comms_ptr->xbee_dev);
    }

    return;
}

/* Function for handling Received Transmit Status Frame */

static int transmit_status_handler( xbee_dev_t *xbee,
   const void FAR *frame, uint16_t length, void FAR *context){

    const xbee_frame_transmit_status_t FAR *payload = frame;

    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(length);
    XBEE_UNUSED_PARAMETER(context);

    /* @todo If success - implement rx/tx stats counter */

    return 0;
}