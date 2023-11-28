
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H
#include <stdint.h>
#include <tx_api.h>
#include <stdatomic.h>

#define ERROR_HANDLER_TX_QUEUE_SIZE 10

typedef enum error_codes{
  CAN_UNPACK_INIT_ERROR,
  CAN_UNPACK_RTCAN_INIT_ERROR,
  CAN_UNPACK_STATS_MUTEX_ERROR,
  CAN_UNPACK_QUEUE_ERROR,
  XBEE_COMMS_INIT_ERROR,
  XBEE_COMMS_QUEUE_ERROR
};

typedef struct {

  TX_THREAD thread;

  TX_QUEUE tx_queue;

  atomic_uint error_code;

  TX_SEMAPHORE fault_semaphore;

} error_handler_context_t;

UINT error_handler_init(error_handler_context_t* error_handler_ptr, TX_BYTE_POOL* stack_pool_ptr);
void critical_error(TX_THREAD *thread_ptr, uint32_t error_code, error_handler_context_t* error_handler);

#endif /* ERROR_HANDLER_H */
