#ifndef CAN_UNPACK_H
#define CAN_UNPACK_H
#include "tx_api.h"
#include "telemetry_protocol.h"
#include "rtcan.h"
#include <stdint.h>
#include "error_handler.h"

#define CAN_PUBLISHER_RX_QUEUE_SIZE 10 //TODO: add config.h storing such values to avoid redefinition.
#define PDU_TX_QUEUE_SIZE 10

typedef struct {

  TX_TIMER bps_timer; /* Timer definition */
  TX_MUTEX stats_mutex; /* Mutex definition */
  int32_t rx_can_count; /* Received can frames */
  int32_t rx_can_bps; /* Received bits per second */
  int32_t rx_bytes;
  int32_t tx_pdu_count; /* Transmitted pdu frames */
  int32_t tx_pdu_bps; /* Transmitted bits per second */
  int32_t tx_bytes;
} unpack_stats_t;

typedef struct {

  TX_THREAD thread;

  TX_QUEUE rx_queue;

  rtcan_msg_t* rx_queue_mem[CAN_PUBLISHER_RX_QUEUE_SIZE];

  TX_QUEUE tx_queue;

  pdu_t* tx_queue_mem[PDU_TX_QUEUE_SIZE];

  rtcan_handle_t* rtcan;

  unpack_stats_t stats;

  error_handler_context_t* error_handler;

} unpack_context_t;

UINT unpack_init(unpack_context_t* unpack_ptr, error_handler_context_t* error_handler_context, TX_BYTE_POOL* stack_pool_ptr, rtcan_handle_t* rtcan);
TX_QUEUE * can_unpack_get_rx_queue_ptr(unpack_context_t* can_unpack_context);
TX_QUEUE * can_unpack_get_tx_queue_ptr(unpack_context_t* can_unpack_context);

#endif /* CAN_UNPACK_H */
