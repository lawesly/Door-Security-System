#ifndef __RC522_H
#define __RC522_H	
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "sys.h"
/*******************************
*连线说明：
*1--SDA  <----->PA4
*2--SCK  <----->PA5
*3--MOSI <----->PA7
*4--MISO <----->PA6
*5--悬空
*6--GND <----->GND
*7--RST <----->PB0
*8--VCC <----->VCC
************************************/

//MF522命令代码
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算

//Mifare_One卡片命令代码
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠

#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte
#define MAXRLEN  18

//MF522寄存器定义
////////////////////////
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivlEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F		  		  0x3F

//和RC522通讯时返回的M1卡状态
#define 	MI_OK                 0x26
#define 	MI_NOTAGERR           0xcc
#define 	MI_ERR                0xbb

//和MF522通讯时返回的错误代码
#define	    SHAQU1                0X01
#define  	KUAI4	              0X04
#define 	KUAI7	              0X07
#define	    REGCARD	              0xa1
#define 	CONSUME	              0xa2
#define     READCARD	          0xa3
#define     ADDMONEY	          0xa4

#define SPI_RC522_ReadByte()	      SPI_RC522_SendByte(uint8_t byte)

#define SET_SPI_CS  (GPIOF->BSRR=0X01)
#define CLR_SPI_CS  (GPIOF->BRR=0X01)

#define SET_RC522RST  GPIOF->BSRR=0X02
#define CLR_RC522RST  GPIOF->BRR=0X02


/***********************RC522 函数宏定义**********************/
#define          RC522_CS_Enable()         PAout(4) = 0
#define          RC522_CS_Disable()        PAout(4) = 1

#define          RC522_Reset_Enable()      PBout(12) = 0
#define          RC522_Reset_Disable()     PBout(12) = 1

#define          RC522_SCK_0()             PAout(5) = 0
#define          RC522_SCK_1()             PAout(5) = 1

#define          RC522_MOSI_0()            PAout(7) = 0
#define          RC522_MOSI_1()            PAout(7) = 1

#define          RC522_MISO_GET()          PAin(6) 

uint8_t       SPI_RC522_SendByte         ( uint8_t byte);
uint8_t       ReadRawRC                  ( uint8_t ucAddress );
void     WriteRawRC                 ( uint8_t ucAddress, uint8_t ucValue );
void     SPI1_Init                  ( void );
void     RC522_Handel               ( void );
void     RC522_Init                 ( void );                       //初始化
void     PcdReset                   ( void );                       //复位
void     M500PcdConfigISOType       ( uint8_t type );                    //工作方式
char     PcdRequest                 ( uint8_t req_code, uint8_t * pTagType ); //寻卡
char     PcdAnticoll                ( uint8_t * pSnr);                   //防冲撞

void     PcdAntennaOn               ( void );                 //开启天线
void     PcdAntennaOff              ( void );                 //关闭天线
void     SetBitMask                 ( uint8_t ucReg, uint8_t ucMask );
void     ClearBitMask               ( uint8_t ucReg, uint8_t ucMask );
char     PcdSelect                  ( uint8_t * pSnr );            //选择卡片
char     PcdAuthState               ( uint8_t ucAuth_mode, uint8_t ucAddr, uint8_t * pKey, uint8_t * pSnr );                                              //验证密码
char     PcdWrite                   ( uint8_t ucAddr, uint8_t * pData );
char     PcdRead                    ( uint8_t ucAddr, uint8_t * pData );
void     ShowID                     ( uint16_t x,uint16_t y, uint8_t *p, uint16_t charColor, uint16_t bkColor);	 //显示卡的卡号，以十六进制显示
char     PcdHalt                    ( void );           //命令卡片进入休眠状态
void     CalulateCRC                ( uint8_t * pIndata, uint8_t ucLen, uint8_t * pOutData );

#endif
