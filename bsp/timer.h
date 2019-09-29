#ifndef __TIMER_H
#define __TIMER_H
#include "stm32f1xx_hal.h"


extern TIM_HandleTypeDef TIM3_Handler;      //¶¨Ê±Æ÷¾ä±ú 


void TIM3_Init(uint16_t arr,uint16_t psc);
void timer3_pwm_Init(uint16_t arr,uint16_t psc);
void TIM_SetTIM3Compare2(uint32_t compare);
#endif

