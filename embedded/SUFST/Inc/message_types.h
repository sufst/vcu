/***************************************************************************
 * @file   messaging.h
 * @author Rory Wilson (rw7g20@soton.ac.uk)
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-12-07
 * @brief  Definitions for message types used by messaging system
 ***************************************************************************/

#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

typedef struct Torque_Request_Message_T {

	UINT priority;
	UINT value;

} Torque_Request_Message;

#endif
