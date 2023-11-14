#ifndef CAN_PUBLISHER_H
#define CAN_PUBLISHER_H
#include <stdint.h>
#include <tx_api.h>
#include "telemetry_protocol.h"

typedef struct {

  TX_THREAD thread;

  TX_QUEUE* tx_queue;

} publisher_context_t;

UINT can_publisher_init(publisher_context_t* publisher_ptr, TX_QUEUE * rx_queue, TX_BYTE_POOL* stack_pool_ptr);

#endif /* CAN_PUBLISHER_H */