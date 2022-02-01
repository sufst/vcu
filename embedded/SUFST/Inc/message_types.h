/***************************************************************************
 * @file   messaging.h
 * @author Rory Wilson (rw7g20@soton.ac.uk)
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-12-07
 * @brief  Definitions for message types and priorities used by messaging system
 ***************************************************************************/

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

/**
 * @brief Enumerated message priority
 */
typedef enum {
	high = 1,
	medium,
	low
} message_priority_t;

/**
 * @brief Demo type for message
 */
typedef struct {

	UINT priority;
	UINT value;

} torque_request_message_t;

#endif
