#ifndef CAN_TELEMETRY_H
#define CAN_TELEMETRY_H

#include <stdint.h>
#include <can_handlers.h>

/**
 * @brief  Frame Header
 * @note 
 * @retval None  
 */

typedef struct header_s
{
    uint16_t valid_bitfield;
    unsigned int    epoch;
} header_t;

/**
 * @brief   
 *
 * @note    
 *          
 */
typedef struct
{
    /**
     * @brief   
     */
    unsigned char start_byte;

    /**
     * @brief   
     */
    uint8_t ID;

    /**
     * @brief   Message data header
     */
//    header_t header;
    uint16_t valid_bitfield;
    unsigned int    epoch;
    /**
     * @brief   Message data buffer
     */
    uint8_t data[8];

} pdu_t;

/**
 * @brief   Table of CAN message IDs and associated unpacking functions
 */
static uint32_t ts_table[CAN_HANDLERS_TABLE_SIZE] = {0};

#endif /* CAN_TELEMETRY_H */