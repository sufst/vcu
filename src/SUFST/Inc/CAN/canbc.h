/***************************************************************************
 * @file    canbc.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Broadcasts state information to a CAN bus using the RTCAN
 *          service
 ***************************************************************************/

#ifndef CANBC_H
#define CANBC_H

#include "tx_api.h"

#include "rtcan.h"

/*
 * channel and segment constants
 */
#define CANBC_MAX_NUM_CHANNELS  10
#define CANBC_MAX_NUM_SEGMENTS  4
#define CANBC_BYTES_PER_SEGMENT 8

/*
 * error codes
 */
#define CANBC_ERROR_NONE         0x00000000U // no error
#define CANBC_ERROR_INIT         0x00000001U // failed to initialise
#define CANBC_ERROR_MEMORY_FULL  0x00000002U // not enough memory to create
#define CANBC_ERROR_SEGMENT_FULL 0x00000004U // no memory left in segment
#define CANBC_ERROR_ARG          0x00000010U // invalid argument
#define CANBC_ERROR_INTERNAL     0x80000000U // internal error

/*
 * CAN broadcast status
 */
typedef enum _canbc_status_t
{
    CANBC_OK,
    CANBC_ERROR
} canbc_status_t;

/**
 * @brief   Broadcast channel
 *
 * @details Represents a single datapoint which can be broadcast
 */

typedef struct _canbc_channel_t
{

    /**
     * @brief   Pointer to the data to be broadcast
     *
     * @details This must not go out of the scope while the channel is activated
     */
    uint8_t* data_ptr;

    /**
     * @brief   Length of data in bytes
     */
    uint32_t data_length;

    /**
     * @brief   Index of first data byte within segment
     */
    uint32_t byte_offset;

    /**
     * @brief   Link to next channel
     */
    struct _canbc_channel_t* next_channel_ptr;

} canbc_channel_t;

/**
 * @brief   Broadcast segment
 *
 * @details Represents a set of channels which can be broadcast in a single
 *          CAN message. Channels are stored as a linked list.
 */
typedef struct _canbc_segment_t
{

    /**
     * @brief   CAN standard ID for broadcast message containing segment
     */
    uint32_t identifier;

    /**
     * @brief   Channel count
     */
    uint32_t bytes_used;

    /**
     * @brief   First channel in the segment, linked to the next channel
     */
    canbc_channel_t* first_channel_ptr;

    /**
     * @brief   Pointer to mutex to be locked when reading data
     *
     * @details NULL pointer will not be locked.
     *          Enabling priority inheritance on this mutex is a good idea.
     */
    TX_MUTEX* mutex_ptr;

    /**
     * @brief   Link to next segment
     */
    struct _canbc_segment_t* next_segment_ptr;

} canbc_segment_t;

/**
 * memory pool sizing constants
 *
 * TODO: what happens if these sizes are truncated due to sizeof broadcast
 *       channel not being a multiple of sizeof(ULONG)
 */
#define CANBC_CHANNEL_POOL_BLOCK_SIZE (sizeof(canbc_channel_t))

#define CANBC_CHANNEL_POOL_SIZE \
    (CANBC_CHANNEL_POOL_BLOCK_SIZE * CANBC_MAX_NUM_CHANNELS)

#define CANBC_SEGMENT_POOL_BLOCK_SIZE (sizeof(canbc_segment_t))

#define CANBC_SEGMENT_POOL_SIZE \
    (CANBC_SEGMENT_POOL_BLOCK_SIZE * CANBC_MAX_NUM_SEGMENTS)

/**
 * @brief   CAN broadcaster
 */
typedef struct _canbc_handle_t
{
    /**
     * @brief   Broadcast service thread
     */
    TX_THREAD thread;

    /**
     * @brief   Broadcast tick timer
     */
    TX_TIMER tick_timer;

    /**
     * @brief   Broadcast period in ticks
     */
    uint32_t tick_period;

    /**
     * @brief   RTCAN service to use for broadcasting
     */
    rtcan_handle_t* rtcan_h;

    /**
     * @brief   Block pool for channels
     */
    TX_BLOCK_POOL channel_pool;

    /**
     * @brief   Memory for channels block pool
     */
    ULONG channel_pool_mem[CANBC_CHANNEL_POOL_SIZE];

    /**
     * @brief   Block pool for segments
     */
    TX_BLOCK_POOL segment_pool;

    /**
     * @brief   Memory for segments block pool
     */
    ULONG segment_pool_mem[CANBC_SEGMENT_POOL_SIZE];

    /**
     * @brief   First segment, linked to the next segment
     */
    canbc_segment_t* first_segment_ptr;

    /**
     * @brief   Current error code
     */
    uint32_t err;

} canbc_handle_t;

/*
 * function prototypes
 */
canbc_status_t canbc_init(canbc_handle_t* canbc_h,
                          rtcan_handle_t* rtcan_h,
                          UINT priority,
                          uint32_t broadcast_period,
                          TX_BYTE_POOL* stack_pool_ptr);

canbc_status_t canbc_start(canbc_handle_t* canbc_h);

canbc_status_t canbc_create_segment(canbc_handle_t* canbc_h,
                                    canbc_segment_t** segment_ptr,
                                    uint32_t identifier,
                                    TX_MUTEX* mutex_ptr);

canbc_status_t canbc_create_channel(canbc_handle_t* canbc_h,
                                    canbc_channel_t** channel_ptr,
                                    canbc_segment_t* segment_ptr,
                                    uint8_t* data_ptr,
                                    uint32_t data_length);

uint32_t canbc_get_error(canbc_handle_t* canbc_h);

#endif