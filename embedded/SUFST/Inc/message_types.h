/***************************************************************************
 * @file   messaging.h
 * @author Rory Wilson (rw7g20@soton.ac.uk)
 * @date   2021-12-07
 * @brief  Definitions for message types used by messaging system
 ***************************************************************************/

typedef struct Torque_Request_Message_T {

	UINT priority;
	UINT value;

} Torque_Request_Message;
