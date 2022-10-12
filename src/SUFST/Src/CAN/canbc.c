/***************************************************************************
 * @file    canbc.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   CAN broadcast implementation
 ***************************************************************************/

#include "canbc.h"

#include <stdbool.h>

#include <memory.h>

/*
 * thread and pool constants
 */
#define CANBC_THREAD_NAME       "CAN Broadcast Thread"
#define CANBC_THREAD_STACK_SIZE 1024 // TODO: this needs to be profiled
#define CANBC_CHANNEL_POOL_NAME "CAN Broadcast Channel Pool"
#define CANBC_SEGMENT_POOL_NAME "CAN Broadcast Segment Pool"
#define CANBC_TICK_TIMER_NAME   "CAN Broadcast Tick Timer"

/*
 * internal functions
 */
static canbc_status_t create_status(canbc_handle_t* canbc_h);

static void create_message(canbc_segment_t* segment_ptr, rtcan_msg_t* msg_ptr);

static bool no_errors(canbc_handle_t* canbc_h);

static void canbc_tick_callback(ULONG input);

static void canbc_thread_entry(ULONG input);

/**
 * @brief       Initialises the CAN broadcast service
 *
 * @param[in]   canbc_h             CAN broadcast handle
 * @param[in]   rtcan_h             RTCAN service handle for broadcasting data
 * @param[in]   priority            Service thread priority
 * @param[in]   broadcast_period    Broadcast period in milliseconds
 * @param[in]   stack_pool_ptr      Memory pool to allocate stack memory from
 */
canbc_status_t canbc_init(canbc_handle_t* canbc_h,
                          rtcan_handle_t* rtcan_h,
                          UINT priority,
                          uint32_t broadcast_period,
                          TX_BYTE_POOL* stack_pool_ptr)
{
    canbc_h->rtcan_h = rtcan_h;
    canbc_h->first_segment_ptr = NULL;

    // timer tick period in ticks
    canbc_h->tick_period
        = (broadcast_period * TX_TIMER_TICKS_PER_SECOND) / 1000;

    if (canbc_h->tick_period == 0)
    {
        canbc_h->err |= CANBC_ERROR_ARG; // too fast!
    }

    // memory pool for channels
    UINT tx_status;

    if (no_errors(canbc_h))
    {
        tx_status = tx_block_pool_create(&canbc_h->channel_pool,
                                         CANBC_CHANNEL_POOL_NAME,
                                         CANBC_CHANNEL_POOL_BLOCK_SIZE,
                                         canbc_h->channel_pool_mem,
                                         CANBC_CHANNEL_POOL_SIZE);
        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    // memory pool for segments
    if (no_errors(canbc_h))
    {
        tx_status = tx_block_pool_create(&canbc_h->segment_pool,
                                         CANBC_SEGMENT_POOL_NAME,
                                         CANBC_SEGMENT_POOL_BLOCK_SIZE,
                                         canbc_h->segment_pool_mem,
                                         CANBC_SEGMENT_POOL_SIZE);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    // service thread
    void* stack_ptr = NULL;

    if (no_errors(canbc_h))
    {
        tx_status = tx_byte_allocate(stack_pool_ptr,
                                     &stack_ptr,
                                     CANBC_THREAD_STACK_SIZE,
                                     TX_NO_WAIT);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    if (no_errors(canbc_h))
    {
        tx_status = tx_thread_create(&canbc_h->thread,
                                     CANBC_THREAD_NAME,
                                     canbc_thread_entry,
                                     (ULONG) canbc_h,
                                     stack_ptr,
                                     CANBC_THREAD_STACK_SIZE,
                                     priority,
                                     priority,
                                     TX_NO_TIME_SLICE,
                                     TX_DONT_START);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    // tick timer
    if (no_errors(canbc_h))
    {
        tx_status = tx_timer_create(&canbc_h->tick_timer,
                                    CANBC_TICK_TIMER_NAME,
                                    canbc_tick_callback,
                                    (ULONG) canbc_h,
                                    canbc_h->tick_period,
                                    canbc_h->tick_period,
                                    TX_NO_ACTIVATE);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    return create_status(canbc_h);
}

/**
 * @brief       Starts the CAN broadcast service
 *
 * @param[in]   canbc_h     CAN broadcast handle
 */
canbc_status_t canbc_start(canbc_handle_t* canbc_h)
{
    if (no_errors(canbc_h))
    {
        UINT tx_status = tx_thread_resume(&canbc_h->thread);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    if (no_errors(canbc_h))
    {
        UINT tx_status = tx_timer_activate(&canbc_h->tick_timer);

        if (tx_status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INIT;
        }
    }

    return create_status(canbc_h);
}

/**
 * @brief       Creates a new segment
 *
 * @param[in]   canbc_h     CAN broadcast handle
 * @param[out]  segment_ptr     Pointer to store address of created segment
 * @param[in]   identifier      Identifier for new segment
 * @param[in]   mutex_ptr       Mutex protecting access to segment
 */
canbc_status_t canbc_create_segment(canbc_handle_t* canbc_h,
                                    canbc_segment_t** segment_ptr,
                                    uint32_t identifier,
                                    TX_MUTEX* mutex_ptr)
{
    canbc_segment_t* new_segment;

    UINT tx_status = tx_block_allocate(&canbc_h->segment_pool,
                                       (void**) &new_segment,
                                       TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        // add the new segment to the linked list
        if (canbc_h->first_segment_ptr == NULL)
        {
            canbc_h->first_segment_ptr = new_segment;
        }
        else
        {
            canbc_segment_t* end_segment_ptr = canbc_h->first_segment_ptr;

            while (end_segment_ptr != NULL)
            {
                end_segment_ptr = end_segment_ptr->next_segment_ptr;
            }
        }

        // write config into segment
        new_segment->identifier = identifier;
        new_segment->mutex_ptr = mutex_ptr;
        new_segment->first_channel_ptr = NULL;
        new_segment->bytes_used = 0;
        new_segment->next_segment_ptr = NULL;

        // update address of pointer parameter
        *segment_ptr = new_segment;
    }
    else
    {
        canbc_h->err |= CANBC_ERROR_MEMORY_FULL;
        *segment_ptr = NULL;
    }

    return create_status(canbc_h);
}

/**
 * @brief       Creates a new channel and adds it to a segment
 *
 * @param[in]   canbc_h     CAN broadcast handle
 * @param[out]  channel_ptr     Pointer to store address of created channel
 * @param[in]   segment_ptr     Pointer to segment to add channel to
 * @param[in]   data_ptr        Pointer to data which channel logs
 * @param[in]   data_length     Length of data which channel logs in bytes
 */
canbc_status_t canbc_create_channel(canbc_handle_t* canbc_h,
                                    canbc_channel_t** channel_ptr,
                                    canbc_segment_t* segment_ptr,
                                    uint8_t* data_ptr,
                                    uint32_t data_length)
{
    canbc_channel_t* new_channel;

    UINT tx_status = tx_block_allocate(&canbc_h->channel_pool,
                                       (void**) &new_channel,
                                       TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        // add the new channel to the linked list in the segment
        if (segment_ptr->first_channel_ptr == NULL)
        {
            segment_ptr->first_channel_ptr = new_channel;
        }
        else
        {
            canbc_channel_t* end_channel_ptr = segment_ptr->first_channel_ptr;

            while (end_channel_ptr != NULL)
            {
                end_channel_ptr = end_channel_ptr->next_channel_ptr;
            }
        }

        // write config into channel
        new_channel->data_ptr = data_ptr;
        new_channel->data_length = data_length;
        new_channel->next_channel_ptr = NULL;
        new_channel->byte_offset = segment_ptr->bytes_used;

        segment_ptr->bytes_used += data_length;
        // TODO: what if this exceeds data_length

        // update address of pointer parameter
        *channel_ptr = new_channel;
    }
    else
    {
        canbc_h->err |= CANBC_ERROR_MEMORY_FULL;
        *channel_ptr = NULL;
    }

    return create_status(canbc_h);
}

/**
 * @brief       Returns the error code
 *
 * @param[in]   canbc_h     CAN broadcast handle
 */
uint32_t canbc_get_error(canbc_handle_t* canbc_h)
{
    return canbc_h->err;
}
/**
 * @brief       Tick timer callback
 *
 * @param[in]   input   CANBC handle
 */
static void canbc_tick_callback(ULONG input)
{
    canbc_handle_t* canbc_h = (canbc_handle_t*) input;

    // restart thread
    if (no_errors(canbc_h))
    {
        UINT status = tx_thread_resume(&canbc_h->thread);

        if (status != TX_SUCCESS)
        {
            canbc_h->err |= CANBC_ERROR_INTERNAL;
        }
    }
}

/**
 * @brief       Entry function for CAN broadcast service thread
 *
 * @param[in]   input  CAN broadcast handle
 */
static void canbc_thread_entry(ULONG input)
{
    canbc_handle_t* canbc_h = (canbc_handle_t*) input;

    while (1)
    {
        // send off each channel for transmission
        canbc_segment_t* segment_ptr = canbc_h->first_segment_ptr;

        while (segment_ptr != NULL)
        {
            rtcan_msg_t message;
            create_message(segment_ptr, &message);

            rtcan_transmit(canbc_h->rtcan_h, &message);

            segment_ptr = segment_ptr->next_segment_ptr;
        }

        // suspend until resumed by next tick
        // TODO: handle error
        (void) tx_thread_suspend(&canbc_h->thread);
    }
}

/**
 * @brief       Populates the RTCAN message with the combined data from all
 *              channels in a segment
 *
 * @param[in]   segment_ptr     Pointer to segment
 * @param[out]  msg_ptr         Pointer to RTCAN message to populate
 */
static void create_message(canbc_segment_t* segment_ptr, rtcan_msg_t* msg_ptr)
{
    bool needs_lock = (segment_ptr->mutex_ptr != NULL);

    msg_ptr->identifier = segment_ptr->identifier;
    msg_ptr->length = segment_ptr->bytes_used;

    if (needs_lock)
    {
        tx_mutex_get(segment_ptr->mutex_ptr, TX_WAIT_FOREVER);
    }

    canbc_channel_t* channel_ptr = segment_ptr->first_channel_ptr;

    while (channel_ptr != NULL)
    {
        memcpy(msg_ptr->data + channel_ptr->byte_offset,
               channel_ptr->data_ptr,
               channel_ptr->data_length);

        channel_ptr = channel_ptr->next_channel_ptr;
    }

    if (needs_lock)
    {
        tx_mutex_put(segment_ptr->mutex_ptr);
    }
}

/**
 * @brief       Returns true if the broadcaster has encountered an error
 *
 * @param[in]   canbc_h     CAN broadcast handle
 */
static bool no_errors(canbc_handle_t* canbc_h)
{
    return (canbc_h->err == CANBC_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   canbc_h     CAN broadcast handle
 */
static canbc_status_t create_status(canbc_handle_t* canbc_h)
{
    return (no_errors(canbc_h)) ? CANBC_OK : CANBC_ERROR;
}