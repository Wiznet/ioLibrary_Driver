/*
 * Timer_Handler.h
 *
 *  Created on: 2020. 11. 16.
 *      Author: Mason_Office
 */

#ifndef TIMER_HANDLER_H_
#define TIMER_HANDLER_H_


#include <sys/time.h>
#include <stdint.h>

void delay_ms(uint32_t ms);
time_t millis(void);

#endif /* TIMER_HANDLER_H_ */
