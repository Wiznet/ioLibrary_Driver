/*
 * Timer_Handler.c
 *
 *  Created on: 2020. 11. 16.
 *      Author: Mason_Office
 */

#include <sys/time.h>
#include "stm32f4xx_hal.h"

volatile uint32_t delaytime_msec = 0;
static volatile time_t devtime_msec = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) //Make 1ms timer interrupt. this example is for STM32F401
{
    if(htim->Instance == TIM2)
    {

        delaytime_msec++;
        devtime_msec++;
    }
}

void delay_ms(uint32_t ms)
{
    uint32_t wakeuptime_msec = delaytime_msec + ms;
    while(wakeuptime_msec > delaytime_msec){}
}

time_t millis(void)
{
    return devtime_msec;
}

