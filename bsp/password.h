#ifndef __PASSWORD_H
#define __PASSWORD_H
#include "stm32f1xx_hal.h"

#define PSW_FLAG	    0xe0
#define PSW_ADD_BASE    0xe1
#define MATCH_PSW       1
#define SET_PSW         0

void psw_check(void);
uint8_t password_setting(void);
void Psd_setting_GUI(uint8_t mode);
uint8_t Get_Touch_KeyValue(void);
uint8_t Get_User_Input_psw(char psw[6]);
uint8_t Match_psw(void);
#endif //_PASSWORD_H
