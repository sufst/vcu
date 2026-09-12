/***************************************************************************
 * @file   rtds.h
 * @author Martin Perreau (@maartin0)
 * @brief  RS232 transciever util for the TRS3221E
 ***************************************************************************/

#ifndef RS232_H
#define RS232_H

#include <stdbool.h>

void rs232_enable(void);
void rs232_disable(void);
bool rs232_is_valid(void);

#endif
