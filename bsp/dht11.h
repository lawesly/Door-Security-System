#ifndef __DS18B20_H
#define __DS18B20_H
#include "sys.h"


//IO��������
#define DHT11_IO_IN()  {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=8<<12;}
#define DHT11_IO_OUT() {GPIOG->CRH&=0XFFFF0FFF;GPIOG->CRH|=3<<12;}

//IO��������											   
#define	DHT11_DQ_OUT    PGout(11)//���ݶ˿�	PG11
#define	DHT11_DQ_IN     PGin(11) //���ݶ˿�	PG11
   	
uint8_t DHT11_Init(void);//��ʼ��DHT11
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);//��ȡ��ʪ��
uint8_t DHT11_Read_Byte(void);//����һ���ֽ�
uint8_t DHT11_Read_Bit(void);//����һ��λ
uint8_t DHT11_Check(void);//����Ƿ����DHT11
void DHT11_Rst(void);//��λDHT11  
#endif