#ifndef __PUBLIC_H
#define __PUBLIC_H

//包含各个模块的头文件
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_sram.h"
#include "sys.h"
#include "stdint.h"
#include <string.h>
#include "usart.h"
#include "bsp_led.h"
#include "rtthread.h"
#include <rtdevice.h>
#include <drv_usart.h>
#include "board.h"
#include "bsp_timer.h"
#include "timer.h"
#include "bsp_key.h"
#include "lcd.h"
#include "beep.h"
#include "remote.h"
#include "24l01.h"
#include "24cxx.h"
#include "ctiic.h"
#include "myiic.h"
#include "touch.h"
#include "lcd.h"
#include "rtc.h"
#include "rc522_config.h"
#include "rc522_function.h"
#include "password.h"
#include <stdlib.h>
/*********WWDG,IWDG开关*********/
#define iwdg_enable 0
#define wwdg_enable 0

/*******微秒延时宏定义******/
#define delayus(x) { uint32_t _dcnt; \
					_dcnt=(x*8); \
					while(_dcnt-- > 0) \
				    { continue; }\
					}
//外部OLED字库变量声明
extern const unsigned char F6x8[][6];
extern const unsigned char F8X16[];
extern const char Hzk1[][32];
extern const unsigned char BMP1[];
extern const unsigned char BMP2[];

/*函数声明*/
void Delay_1US(uint16_t nCount);					
void delay200(uint16_t ms);
void Delay_ms(uint16_t ms);
void Delay1us(uint16_t us);
void delay_2us(uint16_t nCount);
uint16_t Get_decimal(double dt,uint8_t deci);
#endif


