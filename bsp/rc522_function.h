#ifndef __RC522_FUNCTION_H
#define	__RC522_FUNCTION_H
#include "stm32f1xx_hal.h"

/**************卡片操作宏定义************/
#define Mod_Cnt_Tag_Add            0x00              //该地址高两位表示是否修改,10表示修改过，其他数值表示没有修改过
										             //低6位数值为登记的tag数目
#define Base_Add                   0x01    	         //卡片存储基地址							
#define Tag_Stored_Base_Add(cnt) ((cnt*4)-4+Base_Add) //每个新登记的tag ID起始保存地址，
                                                     //根据登记个数计算，起始地址0x01
#define If_Modified                0xc0				 //判断地址0x00高两位是否被修改	
#define Get_Tag_Cnt                0x3f			     //获取地址0x00低6位中的数目
#define Modified                   0x80				 //将地址0x00高两位的修改标志位置位
#define macDummy_Data              0x00

/**************函数声明****************/
void PcdReset( void );                                   //复位
void M500PcdConfigISOType( uint8_t type );               //工作方式
char PcdRequest( uint8_t req_code, uint8_t * pTagType ); //寻卡
char PcdAnticoll( uint8_t * pSnr);                       //读卡号
void SPI_RC522_SendByte ( uint8_t byte );
uint8_t ReadRawRC( uint8_t ucAddress );
void WriteRawRC( uint8_t ucAddress, uint8_t ucValue );
void SPI_RC522_SendByte ( uint8_t byte );
uint8_t SPI_RC522_ReadByte ( void );
uint8_t Read_ID (void);
uint8_t tag_match(uint8_t* tag_id);
uint8_t regist_tag(uint8_t* tag_id);
void Register_New_Tag(void);
void Delete_registered_Tag(void);
#endif /* __RC522_FUNCTION_H */
