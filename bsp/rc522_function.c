#include "rc522_function.h"
#include "rc522_config.h"
#include "stm32f1xx_hal.h"
#include "public.h"

extern rt_sem_t match_thread_lock;
uint8_t ucArray_ID[4];  /*先后存放IC卡的类型和UID(IC卡序列号)*/   
#define   RC522_DELAY()  delay_2us ( 170 )

/**
  * @brief  向RC522发送1 Byte 数据
  * @param  byte，要发送的数据
  * @retval RC522返回的数据
  */
void SPI_RC522_SendByte ( uint8_t byte )
{
  uint8_t counter;

  for(counter=0;counter<8;counter++)
  {     
    if ( byte & 0x80 )
      RC522_MOSI_1 ();
    else 
      RC522_MOSI_0 ();
    
    RC522_DELAY();
    RC522_SCK_0 ();
    
    RC522_DELAY();
    RC522_SCK_1();
    
    RC522_DELAY();
    byte <<= 1; 
  } 	
}


/**
  * @brief  从RC522发送1 Byte 数据
  * @param  无
  * @retval RC522返回的数据
  */
uint8_t SPI_RC522_ReadByte ( void )
{
  uint8_t counter;
  uint8_t SPI_Data;

  for(counter=0;counter<8;counter++)
  {
    SPI_Data <<= 1;
    RC522_SCK_0 ();
   
    RC522_DELAY();
    if ( RC522_MISO_GET() == 1)
     SPI_Data |= 0x01;
    
    RC522_DELAY();
    RC522_SCK_1 ();
    
    RC522_DELAY();
  }
  return SPI_Data;
	
}



/**
  * @brief  读RC522寄存器
  * @param  ucAddress，寄存器地址
  * @retval 寄存器的当前值
  */
uint8_t ReadRawRC ( uint8_t ucAddress )
{
	uint8_t ucAddr, ucReturn;
	
	ucAddr = ( ( ucAddress << 1 ) & 0x7E ) | 0x80;	
	RC522_CS_Enable();
  
	SPI_RC522_SendByte ( ucAddr );
	ucReturn = SPI_RC522_ReadByte();
  
	RC522_CS_Disable();
	
	return ucReturn;	
}

/**
  * @brief  写RC522寄存器
  * @param  ucAddress，寄存器地址
  * @param  ucValue，写入寄存器的值
  * @retval 无
  */
void WriteRawRC ( uint8_t ucAddress, uint8_t ucValue )
{  
	uint8_t ucAddr;
	
	ucAddr = ( ucAddress << 1 ) & 0x7E;	
	RC522_CS_Enable();
	
	SPI_RC522_SendByte ( ucAddr );	
	SPI_RC522_SendByte ( ucValue );
  
	RC522_CS_Disable();		
}


/**
  * @brief  对RC522寄存器置位
  * @param  ucReg，寄存器地址
  * @param   ucMask，置位值
  * @retval 无
  */
void SetBitMask ( uint8_t ucReg, uint8_t ucMask )  
{
  uint8_t ucTemp;

  ucTemp = ReadRawRC ( ucReg );
  WriteRawRC ( ucReg, ucTemp | ucMask ); // set bit mask
}


/**
  * @brief  对RC522寄存器清位
  * @param  ucReg，寄存器地址
  * @param  ucMask，清位值
  * @retval 无
  */
void ClearBitMask ( uint8_t ucReg, uint8_t ucMask )  
{
  uint8_t ucTemp;

  ucTemp = ReadRawRC ( ucReg );
  WriteRawRC ( ucReg, ucTemp & ( ~ ucMask) ); // clear bit mask
}


/**
  * @brief  开启天线 
  * @param  无
  * @retval 无
  */
void PcdAntennaOn ( void )
{
  uint8_t uc;

  uc = ReadRawRC ( TxControlReg );
  if ( ! ( uc & 0x03 ) )
   SetBitMask(TxControlReg, 0x03);		
}


/**
  * @brief  关闭天线
  * @param  无
  * @retval 无
  */
void PcdAntennaOff ( void )
{
  ClearBitMask ( TxControlReg, 0x03 );	
}


/**
  * @brief  复位RC522 
  * @param  无
  * @retval 无
  */
void PcdReset ( void )
{
	RC522_Reset_Disable();
	Delay1us ( 10 );
	RC522_Reset_Enable();
	Delay1us ( 10 );
	RC522_Reset_Disable();
	Delay1us (10 );
	WriteRawRC ( CommandReg, 0x0f );
	while ( ReadRawRC ( CommandReg ) & 0x10 );
	Delay1us ( 10 );
	//定义发送和接收常用模式 和Mifare卡通讯，CRC初始值0x6363
    WriteRawRC ( ModeReg, 0x3D );   
    
    WriteRawRC ( TReloadRegL, 30 );      //16位定时器低位    
	WriteRawRC ( TReloadRegH, 0 );		 //16位定时器高位
    WriteRawRC ( TModeReg, 0x8D );	     //定义内部定时器的设置
    WriteRawRC ( TPrescalerReg, 0x3E );	 //设置定时器分频系数
	WriteRawRC ( TxAutoReg, 0x40 );	     //调制发送信号为100%ASK	
}



/**
  * @brief  设置RC522的工作方式
  * @param  ucType，工作方式
  * @retval 无
  */
void M500PcdConfigISOType ( uint8_t ucType )
{
  if ( ucType == 'A')                     //ISO14443_A
  {
		ClearBitMask ( Status2Reg, 0x08 );
		
        WriteRawRC ( ModeReg, 0x3D );         //3F
		
		WriteRawRC ( RxSelReg, 0x86 );        //84
		
		WriteRawRC( RFCfgReg, 0x7F );         //4F
		
		WriteRawRC( TReloadRegL, 30 );        
		
		WriteRawRC ( TReloadRegH, 0 );
		
		WriteRawRC ( TModeReg, 0x8D );
		
		WriteRawRC ( TPrescalerReg, 0x3E );
		
		Delay1us ( 5 );
		
		PcdAntennaOn ();//开天线
		
   }	 
}



/**
  * @brief  通过RC522和ISO14443卡通讯
  * @param  ucCommand，RC522命令字
  * @param  pInData，通过RC522发送到卡片的数据
  * @param  ucInLenByte，发送数据的字节长度
  * @param  pOutData，接收到的卡片返回数据
  * @param  pOutLenBit，返回数据的位长度
  * @retval 状态值= MI_OK，成功
  */
char PcdComMF522 ( uint8_t ucCommand,
                   uint8_t * pInData, 
                   uint8_t ucInLenByte, 
                   uint8_t * pOutData,
                   uint32_t * pOutLenBit )		
{
  char cStatus = MI_ERR;
  uint8_t ucIrqEn   = 0x00;
  uint8_t ucWaitFor = 0x00;
  uint8_t ucLastBits;
  uint8_t ucN;
  uint32_t ul;

  switch ( ucCommand )
  {
     case PCD_AUTHENT:		  //Mifare认证
        ucIrqEn   = 0x12;		//允许错误中断请求ErrIEn  允许空闲中断IdleIEn
        ucWaitFor = 0x10;		//认证寻卡等待时候 查询空闲中断标志位
        break;
     
     case PCD_TRANSCEIVE:		//接收发送 发送接收
        ucIrqEn   = 0x77;		//允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
        ucWaitFor = 0x30;		//寻卡等待时候 查询接收中断标志位与 空闲中断标志位
        break;
     
     default:
       break;     
  }
  //IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反 
  WriteRawRC ( ComIEnReg, ucIrqEn | 0x80 );
  //Set1该位清零时，CommIRqReg的屏蔽位清零
  ClearBitMask ( ComIrqReg, 0x80 );	 
  //写空闲命令
  WriteRawRC ( CommandReg, PCD_IDLE );		 
  
  //置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除
  SetBitMask ( FIFOLevelReg, 0x80 );			

  for ( ul = 0; ul < ucInLenByte; ul ++ )
    WriteRawRC ( FIFODataReg, pInData [ ul ] ); //写数据进FIFOdata
    
  WriteRawRC ( CommandReg, ucCommand );					//写命令


  if ( ucCommand == PCD_TRANSCEIVE )
    
    //StartSend置位启动数据发送 该位与收发命令使用时才有效
    SetBitMask(BitFramingReg,0x80);  				  

  ul = 1000;                             //根据时钟频率调整，操作M1卡最大等待时间25ms

  do 														         //认证 与寻卡等待时间	
  {
       ucN = ReadRawRC ( ComIrqReg );		 //查询事件中断
       ul --;
  } while ( ( ul != 0 ) && ( ! ( ucN & 0x01 ) ) && ( ! ( ucN & ucWaitFor ) ) );	

  ClearBitMask ( BitFramingReg, 0x80 );	 //清理允许StartSend位

  if ( ul != 0 )
  {
    //读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
    if ( ! ( ReadRawRC ( ErrorReg ) & 0x1B ) )	
    {
      cStatus = MI_OK;
      
      if ( ucN & ucIrqEn & 0x01 )				//是否发生定时器中断
        cStatus = MI_NOTAGERR;   
        
      if ( ucCommand == PCD_TRANSCEIVE )
      {
        //读FIFO中保存的字节数
        ucN = ReadRawRC ( FIFOLevelReg );		          
        
        //最后接收到得字节的有效位数
        ucLastBits = ReadRawRC ( ControlReg ) & 0x07;	
        
        if ( ucLastBits )
          
          //N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
          * pOutLenBit = ( ucN - 1 ) * 8 + ucLastBits;   	
        else
          * pOutLenBit = ucN * 8;      //最后接收到的字节整个字节有效
        
        if ( ucN == 0 )		
          ucN = 1;    
        
        if ( ucN > MAXRLEN )
          ucN = MAXRLEN;   
        
        for ( ul = 0; ul < ucN; ul ++ )
          pOutData [ ul ] = ReadRawRC ( FIFODataReg );   
        
        }        
    }   
    else
      cStatus = MI_ERR;       
  }

  SetBitMask ( ControlReg, 0x80 );           // stop timer now
  WriteRawRC ( CommandReg, PCD_IDLE ); 
   
  return cStatus;
}

/**
  * @brief 寻卡
  * @param  ucReq_code，寻卡方式 = 0x52，寻感应区内所有符合14443A标准的卡；
            寻卡方式= 0x26，寻未进入休眠状态的卡
  * @param  pTagType，卡片类型代码
             = 0x4400，Mifare_UltraLight
             = 0x0400，Mifare_One(S50)
             = 0x0200，Mifare_One(S70)
             = 0x0800，Mifare_Pro(X))
             = 0x4403，Mifare_DESFire
  * @retval 状态值= MI_OK，成功
  */
char PcdRequest ( uint8_t ucReq_code, uint8_t * pTagType )
{
  char cStatus;  
  uint8_t ucComMF522Buf [ MAXRLEN ]; 
  uint32_t ulLen;

  //清理指示MIFARECyptol单元接通以及所有卡的数据通信被加密的情况
  ClearBitMask ( Status2Reg, 0x08 );
	//发送的最后一个字节的 七位
  WriteRawRC ( BitFramingReg, 0x07 );
  //TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号
  SetBitMask ( TxControlReg, 0x03 );	

  ucComMF522Buf [ 0 ] = ucReq_code;		//存入 卡片命令字

  cStatus = PcdComMF522 ( PCD_TRANSCEIVE,	
                          ucComMF522Buf,
                          1, 
                          ucComMF522Buf,
                          &ulLen );	//寻卡  
  if ( ( cStatus == MI_OK ) && ( ulLen == 0x10 ) )	//寻卡成功返回卡类型 
  {    
     * pTagType = ucComMF522Buf [ 0 ];
     * ( pTagType + 1 ) = ucComMF522Buf [ 1 ];
  }
  else
   cStatus = MI_ERR;
   return cStatus;	 
}

/**
  * @brief  防冲撞
  * @param  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdAnticoll ( uint8_t * pSnr )
{
  char cStatus;
  uint8_t uc, ucSnr_check = 0;
  uint8_t ucComMF522Buf [ MAXRLEN ]; 
  uint32_t ulLen;
  
  //清MFCryptol On位 只有成功执行MFAuthent命令后，该位才能置位
  ClearBitMask ( Status2Reg, 0x08 );
  //清理寄存器 停止收发
  WriteRawRC ( BitFramingReg, 0x00);	
	//清ValuesAfterColl所有接收的位在冲突后被清除
  ClearBitMask ( CollReg, 0x80 );			  
 
  ucComMF522Buf [ 0 ] = 0x93;	          //卡片防冲突命令
  ucComMF522Buf [ 1 ] = 0x20;
 
  cStatus = PcdComMF522 ( PCD_TRANSCEIVE, 
                          ucComMF522Buf,
                          2, 
                          ucComMF522Buf,
                          & ulLen);      //与卡片通信

  if ( cStatus == MI_OK)		            //通信成功
  {
    for ( uc = 0; uc < 4; uc ++ )
    {
       * ( pSnr + uc )  = ucComMF522Buf [ uc ]; //读出UID
       ucSnr_check ^= ucComMF522Buf [ uc ];
    }
    
    if ( ucSnr_check != ucComMF522Buf [ uc ] )
      cStatus = MI_ERR;    				 
  }
  
  SetBitMask ( CollReg, 0x80 );
      
  return cStatus;		
}


/**
  * @brief  用RC522计算CRC16
  * @param  pIndata，计算CRC16的数组
  * @param  ucLen，计算CRC16的数组字节长度
  * @param  pOutData，存放计算结果存放的首地址
  * @retval 无
  */
void CalulateCRC ( uint8_t * pIndata, 
                 uint8_t ucLen, 
                 uint8_t * pOutData )
{
  uint8_t uc, ucN;

  ClearBitMask(DivIrqReg,0x04);

  WriteRawRC(CommandReg,PCD_IDLE);

  SetBitMask(FIFOLevelReg,0x80);

  for ( uc = 0; uc < ucLen; uc ++)
    WriteRawRC ( FIFODataReg, * ( pIndata + uc ) );   

  WriteRawRC ( CommandReg, PCD_CALCCRC );

  uc = 0xFF;

  do 
  {
      ucN = ReadRawRC ( DivIrqReg );
      uc --;
  } while ( ( uc != 0 ) && ! ( ucN & 0x04 ) );
  
  pOutData [ 0 ] = ReadRawRC ( CRCResultRegL );
  pOutData [ 1 ] = ReadRawRC ( CRCResultRegM );		
}


/**
  * @brief  选定卡片
  * @param  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdSelect ( uint8_t * pSnr )
{
  char ucN;
  uint8_t uc;
  uint8_t ucComMF522Buf [ MAXRLEN ]; 
  uint32_t  ulLen;
  ucComMF522Buf [ 0 ] = PICC_ANTICOLL1;
  ucComMF522Buf [ 1 ] = 0x70;
  ucComMF522Buf [ 6 ] = 0;

  for ( uc = 0; uc < 4; uc ++ )
  {
    ucComMF522Buf [ uc + 2 ] = * ( pSnr + uc );
    ucComMF522Buf [ 6 ] ^= * ( pSnr + uc );
  }
  
  CalulateCRC ( ucComMF522Buf, 7, & ucComMF522Buf [ 7 ] );

  ClearBitMask ( Status2Reg, 0x08 );

  ucN = PcdComMF522 ( PCD_TRANSCEIVE,
                     ucComMF522Buf,
                     9,
                     ucComMF522Buf, 
                     & ulLen );
  
  if ( ( ucN == MI_OK ) && ( ulLen == 0x18 ) )
    ucN = MI_OK;  
  else
    ucN = MI_ERR;    
  
  return ucN;		
}



/**
  * @brief  验证卡片密码
  * @param  ucAuth_mode，密码验证模式= 0x60，验证A密钥，
            密码验证模式= 0x61，验证B密钥
  * @param  uint8_t ucAddr，块地址
  * @param  pKey，密码 
  * @param  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
char PcdAuthState ( uint8_t ucAuth_mode, 
                    uint8_t ucAddr, 
                    uint8_t * pKey,
                    uint8_t * pSnr )
{
  char cStatus;
  uint8_t uc, ucComMF522Buf [ MAXRLEN ];
  uint32_t ulLen;
  

  ucComMF522Buf [ 0 ] = ucAuth_mode;
  ucComMF522Buf [ 1 ] = ucAddr;

  for ( uc = 0; uc < 6; uc ++ )
    ucComMF522Buf [ uc + 2 ] = * ( pKey + uc );   

  for ( uc = 0; uc < 6; uc ++ )
    ucComMF522Buf [ uc + 8 ] = * ( pSnr + uc );   

  cStatus = PcdComMF522 ( PCD_AUTHENT,
                          ucComMF522Buf, 
                          12,
                          ucComMF522Buf,
                          & ulLen );

  if ( ( cStatus != MI_OK ) || ( ! ( ReadRawRC ( Status2Reg ) & 0x08 ) ) )
    cStatus = MI_ERR;   
    
  return cStatus;
}


/**
  * @brief  写数据到M1卡一块
  * @param  uint8_t ucAddr，块地址
  * @param  pData，写入的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
char PcdWrite ( uint8_t ucAddr, uint8_t * pData )
{
  char cStatus;
  uint8_t uc, ucComMF522Buf [ MAXRLEN ];
  uint32_t ulLen;
   
  
  ucComMF522Buf [ 0 ] = PICC_WRITE;
  ucComMF522Buf [ 1 ] = ucAddr;

  CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );

  cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                          ucComMF522Buf,
                          4, 
                          ucComMF522Buf,
                          & ulLen );

  if ( ( cStatus != MI_OK ) || ( ulLen != 4 ) || 
         ( ( ucComMF522Buf [ 0 ] & 0x0F ) != 0x0A ) )
    cStatus = MI_ERR;   
      
  if ( cStatus == MI_OK )
  {
    //memcpy(ucComMF522Buf, pData, 16);
    for ( uc = 0; uc < 16; uc ++ )
      ucComMF522Buf [ uc ] = * ( pData + uc );  
    CalulateCRC ( ucComMF522Buf, 16, & ucComMF522Buf [ 16 ] );

    cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                           ucComMF522Buf, 
                           18, 
                           ucComMF522Buf,
                           & ulLen );
    if ( ( cStatus != MI_OK ) || ( ulLen != 4 ) || 
         ( ( ucComMF522Buf [ 0 ] & 0x0F ) != 0x0A ) )
      cStatus = MI_ERR;    
  } 	
  return cStatus;		
}


/**
  * @brief  读取M1卡一块数据
  * @param  ucAddr，块地址
  * @param  pData，读出的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
char PcdRead ( uint8_t ucAddr, uint8_t * pData )
{
  char cStatus;
  uint8_t uc, ucComMF522Buf [ MAXRLEN ]; 
  uint32_t ulLen;
  ucComMF522Buf [ 0 ] = PICC_READ;
  ucComMF522Buf [ 1 ] = ucAddr;
  CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );
  cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                          ucComMF522Buf,
                          4, 
                          ucComMF522Buf,
                          & ulLen );
  if ( ( cStatus == MI_OK ) && ( ulLen == 0x90 ) )
  {
    for ( uc = 0; uc < 16; uc ++ )
      * ( pData + uc ) = ucComMF522Buf [ uc ];   
  }
  else
    cStatus = MI_ERR;   
  return cStatus;		
}


/**
  * @brief  命令卡片进入休眠状态
  * @param  无
  * @retval 状态值= MI_OK，成功
  */
char PcdHalt( void )
{
	uint8_t ucComMF522Buf [ MAXRLEN ]; 
	uint32_t  ulLen;
    ucComMF522Buf [ 0 ] = PICC_HALT;
    ucComMF522Buf [ 1 ] = 0;
    CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );
 	PcdComMF522 ( PCD_TRANSCEIVE,
                ucComMF522Buf,
                4, 
                ucComMF522Buf, 
                & ulLen );
  return MI_OK;	
}


void IC_CMT ( uint8_t * UID,
              uint8_t * KEY,
              uint8_t RW,
              uint8_t * Dat )
{
  uint8_t ucArray_ID [ 4 ] = { 0 }; //先后存放IC卡的类型和UID(IC卡序列号)	
  PcdRequest ( 0x52, ucArray_ID ); //寻卡
  PcdAnticoll ( ucArray_ID );      //防冲撞
  PcdSelect ( UID );               //选定卡
  PcdAuthState ( 0x60, 0x10, KEY, UID );//校验
	if ( RW )                        //读写选择，1是读，0是写
    PcdRead ( 0x10, Dat );
   else 
     PcdWrite ( 0x10, Dat );
   PcdHalt ();	 
}

/* 函数名：Read_ID
 * 描述  ：读取tag ID
 * 输入  ：无
 * 返回  : MI_OK：读取成功
		   MI_ERR：读取失败
		   MI_NOTAGERR：没有获取到卡片ID
 * 调用  ：外部调用              
*/
uint8_t Read_ID (void)
{
	char cStr [ 30 ];  
	uint8_t ucStatusReturn;    /*返回状态*/                                                                                         
	//while ( 1 )
	//{ 
		/*寻卡*/
		if ( ( ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID ) ) != MI_OK )  
		/*若失败再次寻卡*/
			ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID );	
		if ( ucStatusReturn == MI_OK  )
		{
		     /*防冲撞（当有多张卡进入读写器操作范围时，防冲突机制会从其中选择一张进行操作）*/
			if ( PcdAnticoll ( ucArray_ID ) == MI_OK )                                                                   
			{
				  LED_Blink();
				  sprintf ( cStr, "The Card ID is: %02X%02X%02X%02X       ",
                  ucArray_ID [ 0 ], 
                  ucArray_ID [ 1 ], 
                  ucArray_ID [ 2 ], 
                  ucArray_ID [ 3 ]);
				  printf ( "%s\r\n",cStr ); 
				  LCD_ShowString( 0, 80,240,240,16 ,"                            ");
				  LCD_ShowString( 0, 80,240,240,16 ,cStr);
			    return MI_OK;
			}
			else 
			return MI_ERR;			
		}
		else
		return MI_NOTAGERR;
 // }	
}

/* 函数名：tag_match
 * 描述  ：判断当前tag是否有登记过
 * 输入  ：读取到的tag ID
 * 返回  : 1：有登记，0：没有登记
 * 调用  ：外部调用              
*/
uint8_t tag_match(uint8_t* tag_id)
{
	char target_tag_id[16];//16个数值转换为字符串后加上结束符一中需要16个字节
    char stored_tag_id[16];
	uint8_t cnt,j=0;
	//将读到的ID转换为字符串
	sprintf(target_tag_id,"%02X%02X%02X%02X",*(tag_id),*(tag_id+1),*(tag_id+2),*(tag_id+3));
	printf("target_tag_id:%s\n",target_tag_id);
	//获取当前登记的ID数目
	cnt = AT24CXX_ReadOneByte(Mod_Cnt_Tag_Add)&Get_Tag_Cnt;
	printf("Matching cnt :%d\n",cnt);
	if(cnt==0)
	{
		printf("No tag is registered!\n");
		POINT_COLOR = RED;
		LCD_ShowString( 25, 200,240,240,16 ,"No tag registered!          ");
		POINT_COLOR = BLACK;
		return 0;
	}
	for(j=1;j<=cnt;j++)
	{
		//将登记的ID逐个读出并转换为字符串
		sprintf(stored_tag_id,"%02X%02X%02X%02X",AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+0),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+1),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+2),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+3));
		printf("Matching stored_tag_id %d:%s\n",j,stored_tag_id);
		//然后将读到ID与登记的ID进行逐个比较
		if(strcmp(stored_tag_id,target_tag_id)==0)
		{
			printf("Matched!ID:\n");
			printf("Stored at add:0x%02x,ID is:%s\n",Tag_Stored_Base_Add(j),stored_tag_id);
			return 1;
		}
	}
	printf("ID: %s is Not Registered!\n",target_tag_id);
	POINT_COLOR = RED;
	LCD_ShowString(20, 200,240,240,16 ,"                             ");
	LCD_ShowString(25, 200,240,240,16 ,"Not registered tag!          ");
	POINT_COLOR = BLACK;
	return 0;
}

/* 函数名：regist_tag
 * 描述  ：登记卡片
 * 输入  ：读取到的tag ID
 * 返回  : 1
 * 调用  ：外部调用              
*/
uint8_t regist_tag(uint8_t* tag_id)
{
	uint8_t cnt,i=0;
	//读取当前以登记的卡片数目
	printf("Registering Tag........!\n");
	cnt = AT24CXX_ReadOneByte(Mod_Cnt_Tag_Add)&Get_Tag_Cnt;
	//新登记一个卡片将数目在原来的基础上加一
	cnt += 1;
	//超过最大数清空数目
	if(cnt>56)
	  {
		cnt = 0&Get_Tag_Cnt;
		printf("Storage is full! New ID will overwrite old ID!\n");
	  }
	printf("regist cnt:%d\n",cnt);
	//再将数目写回EEPROM地址0x00中的低六位
	AT24CXX_WriteOneByte(Mod_Cnt_Tag_Add,cnt|Modified); 
	//将新卡片ID写入EEPROM
	for(i=0;i<4;i++)
	{
		//Tag_Stored_Base_Add(cnt)为新卡片ID存储地址偏移，起始地址0x01,根据cnt计算
		AT24CXX_WriteOneByte((Tag_Stored_Base_Add(cnt)+i),*(tag_id+i));
	}
	printf("Tag ID:%02X%02X%02X%02X is Registered\n",*(tag_id+0),*(tag_id+1),*(tag_id+2),*(tag_id+3));
	printf("Stored at address:0x%02x\n",Tag_Stored_Base_Add(cnt));
	return 1;
}

/* 函数名：delete_registered_tag
 * 描述  ：删除系统中指定的卡片ID
 * 输入  ：读取到的tag ID
 * 返回  : 1：成功删除登记的卡片，0：没有登记的卡片要删除
 * 调用  ：外部调用              
*/
uint8_t delete_registered_tag(uint8_t* tag_id)
{
	char target_tag_id[16];//16个数值转换为字符串后加上结束符一中需要16个字节
    char stored_tag_id[16];
	int8_t cnt,j,k,x,y,z,tem=0;
	//将读到的ID转换为字符串
	sprintf(target_tag_id,"%02X%02X%02X%02X",*(tag_id),*(tag_id+1),*(tag_id+2),*(tag_id+3));
	//获取系统中登记的卡片ID总数
	cnt = AT24CXX_ReadOneByte(Mod_Cnt_Tag_Add)&Get_Tag_Cnt;
	k= cnt;
	printf("delete cnt:%d\n",cnt);
	if(cnt==0)
	{
		printf("No tags to be deleted!\n");
		POINT_COLOR = RED;
		LCD_ShowString( 20, 200,240,240,16 ,"                                ");
		LCD_ShowString( 25, 200,240,240,16 ,"No tags to be deleted!          ");
		POINT_COLOR = BLACK;
		return 0;
	}
	printf("Delete Tag is in process....\n");
	//Tag_Stored_Base_Add(cnt)中的cnt不能0,所以此处的J初始值不能为0，否则无法正确获得对应地址中的值
	for(j=1;j<=cnt;j++)
	{
		//将登记的ID逐个读出并转换为字符串
		sprintf(stored_tag_id,"%02X%02X%02X%02X",AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+0),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+1),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+2),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+3));
		printf("Matching stored_tag_id %d:%s\n",j,stored_tag_id);
		//然后将读到ID与登记的ID进行逐个比较
		if(strcmp(stored_tag_id,target_tag_id)==0)
		{
			z=j;//记录被删ID位置
		    printf("Deleting The Tag....\n");
			LCD_ShowString( 25, 200,240,240,16 ,"Deleting The Tag....");
			for(uint8_t i=0;i<4;i++)
			{
				//清除ID
				AT24CXX_WriteOneByte((Tag_Stored_Base_Add(j)+i),0x00);
			}
			printf("Deleted! \n");
			//记录删除多少个ID
			k  = k-1;
		}
	}
	if(k==cnt)
	{
		printf("Target Tag Not found!\n");
		POINT_COLOR = RED;
		LCD_ShowString(20, 200,240,240,16 ,"                             ");
		LCD_ShowString( 25, 200,240,240,16 ,"Target Tag Not found!       ");
		POINT_COLOR = BLACK;
		return 0;
	}
	else
	{
		/**************将被删除ID位置之后的ID填充到当前位置****************/
		for(x=0;x<=(cnt-z);x++)
		{
			printf("transfering data to EEPROM.....\n");
			for(y=0;y<4;y++)
			{
				//x要移动的ID数目，y对应ID的字节数
				tem = AT24CXX_ReadOneByte((((z+x+1)*4)-4+Base_Add)+y);
				AT24CXX_WriteOneByte(((((z+x)*4)-4+Base_Add)+y),tem);
				tem =0;
			}
		}
		cnt=k;
		//再将新的数目写回EEPROM地址0x00中的低六位
		AT24CXX_WriteOneByte(Mod_Cnt_Tag_Add,cnt|Modified); 
		printf("Delete tag process is completed!\n");
		POINT_COLOR = RED;
		LCD_ShowString(20, 200,240,240,16 ,"                          ");
		LCD_ShowString( 50, 200,240,240,16 ,"   Delete Done!         ");
		POINT_COLOR = BLACK;
		return  1;
	}
}
/* 函数名：Register_New_Tag
 * 描述  ：注册新卡到系统，如果已经注册过不执行注册操作
 * 输入  ：无
 * 返回  : 无
 * 调用  ：外部调用              
*/
void Register_New_Tag(void)
{
	//触屏按下LED闪烁一次
    LED_Blink();
	LCD_Clear(WHITE);
	register_delete_GUI(REGISTER);
	loop:	
	//显示提示信息
	LCD_ShowString( 20, 200,240,240,16 ,"                             ");
	while(1)
	{
			   //触屏扫描
			   tp_dev.scan(0); 
			   //判断是否有按键按下
			   if(tp_dev.sta&TP_PRES_DOWN)			
			   {	
					//检测触摸范围是否在液晶显示尺寸之内
					if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height) 
					{
						//消除触摸抖动  
						while(tp_dev.sta&TP_PRES_DOWN)	
						{
							tp_dev.scan(0); 
						}
						//提示音
						beeponce(10);
						//触屏按下LED闪烁一次
						LED_Blink();
						//如果返回键按下则返回正常模式
						if(Back)
							goto loop2;
						//如果NEW按下就再执行一次注册新卡操作
						if(New)
						{
							goto loop;	
						}
						
					}
				}
			LCD_ShowString( 0, 80,280,280,16 ,"Put the tag on the antana area         ");
			//读取TAG ID
			switch(Read_ID())
			{
				//读取成功
				case MI_OK:
							{   
								//刷卡提示音
								beeponce(15);
								//显示操作选项Back 和 New
								//Show_BN;
								//先判断将要注册的卡片之前是否注册过，注册过就不再注册
								if(tag_match(ucArray_ID)==1)
								{
									POINT_COLOR = RED;
									LCD_ShowString(20, 200,240,240,16 ,"                          ");
									LCD_ShowString(40, 200,240,240,16 ,"Already Registered!");
									printf("Already Registerd!\nRegistering rounting aborted!\n");
									POINT_COLOR = BLACK;
								}
								//没有注册过就注册卡片
								else
								{
									regist_tag(ucArray_ID);
									POINT_COLOR = RED;
									LCD_ShowString(20, 200,240,240,16 ,"                             ");
									LCD_ShowString(40, 200,240,240,16 ,"Registered Successfully!");
									POINT_COLOR = BLACK;
								}
								//判断用户选择：返回还是继续登记新卡片
								while(1)
								{
								   //触屏扫描
								   tp_dev.scan(0); 
								   //判断是否有按键按下
								   if(tp_dev.sta&TP_PRES_DOWN)			
									{
										//检测触摸范围是否在液晶显示尺寸之内
										if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height) 
										{
											beeponce(10);
											LED_Blink();
											if(Back)
												goto loop2;
											if(New)
											{
												goto loop;	
											}
											//消除触摸抖动  
											while(tp_dev.sta&TP_PRES_DOWN)	
											{
												tp_dev.scan(0); 
											}
										}
									}
								 }
							}
				default:  
							{ 
									goto loop;	
							}
			}
		}
		  loop2:
		  //恢复主界面
		  security_door_window();		
}

/* 函数名：Delete_registered_Tag
 * 描述  ：删除已登记的卡片
 * 输入  ：无
 * 返回  : 无
 * 调用  ：外部调用              
*/
void Delete_registered_Tag(void)
{
	//触屏按下LED闪烁一次
    LED_Blink();
	LCD_Clear(WHITE);
	register_delete_GUI(DELETE);
	loop:
	//显示提示信息
	LCD_ShowString( 20, 200,240,240,16 ,"                                  ");
	LCD_ShowString( 0, 80,280,280,16 ,"Put the tag on the antana area        ");
	while(1)
	{
		   //触屏扫描
		   tp_dev.scan(0); 
		   //判断是否有按键按下
		   if(tp_dev.sta&TP_PRES_DOWN)			
			{	
				//检测触摸范围是否在液晶显示尺寸之内
				if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height) 
				{
					//消除触摸抖动  
					while(tp_dev.sta&TP_PRES_DOWN)	
					{
						tp_dev.scan(0); 
					}
					//提示音
					beeponce(10);
					//触屏按下LED闪烁一次
					LED_Blink();
					//如果返回键按下则返回正常模式
					if(Back)
						goto loop2;
					//如果NEW按下就再执行一次注册新卡操作
					if(New)
					{
						goto loop;	
					}
					
				}
			}		
		
		//读取TAG ID
		switch(Read_ID())
		{
			//读取成功
			case MI_OK:
						{   
							//提示音
							beeponce(15);
							delete_registered_tag(ucArray_ID);
							//判断用户选择：返回还是继续登记新卡片
							while(1)
							{
							   //触屏扫描
							   tp_dev.scan(0); 
							   //判断是否有按键按下
							   if(tp_dev.sta&TP_PRES_DOWN)			
								{
									//检测触摸范围是否在液晶显示尺寸之内
									if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height) 
									{
										
										beeponce(10);
										//LED闪烁
										LED_Blink();
										if(Back)
											//退出
											goto loop2;
										if(New)
										{
											//重新开始
											goto loop;	
										}
										//消除触摸抖动  
										while(tp_dev.sta&TP_PRES_DOWN)	
										{
											tp_dev.scan(0); 
										}
									}
								}
							 }
						}
			default:  
						{ 
								goto loop;	
						}
		}
	}
	  loop2:
	  //恢复主界面
	  security_door_window();		
}


/* 函数名：DEL_ALL_TAG
 * 描述  ：通过PC串口发送msh命令删除已登记的所有卡片
 * 输入  ：无
 * 返回  : 无
 * 调用  ：外部调用              
*/
uint8_t DEL_ALL_TAG(void)
{
	uint8_t cnt,i,j = 0;
	cnt = AT24CXX_ReadOneByte(Mod_Cnt_Tag_Add)&Get_Tag_Cnt;
	printf("delete cnt:%d\n",cnt);
	if(cnt==0)
	{
		printf("No tags to be deleted!\n");
		return 0;
	}
	for(j=1;j<=cnt;j++)
	{
		for(i=0;i<4;i++)
		{
			printf("Deleting......\n");
			AT24CXX_WriteOneByte((Tag_Stored_Base_Add(j)+i),0x00);
		}
	}
	cnt = 0;
	AT24CXX_WriteOneByte(Mod_Cnt_Tag_Add,cnt|Modified); 
	printf("All registered tags have been deleted!\n");
	return 1;
}
MSH_CMD_EXPORT(DEL_ALL_TAG,delet_all_tags);


/* 函数名：LIST_ALL_TAGS
 * 描述  ：通过PC串口发送msh命令列出所有已登记的卡片ID
 * 输入  ：无
 * 返回  : 无
 * 调用  ：外部调用              
*/
uint8_t LIST_ALL_TAGS(void)
{
	uint8_t cnt,j = 0;
	char stored_tag_id[16];
	cnt = AT24CXX_ReadOneByte(Mod_Cnt_Tag_Add)&Get_Tag_Cnt;
	printf("Total registered Tag ID:%d\n",cnt);
	if(cnt==0)
	{
		printf("No tag is registered!\n");
		return 0;
	}
	for(j=1;j<=cnt;j++)
	{
			//将登记的ID逐个读出并转换为字符串
			sprintf(stored_tag_id,"%02X%02X%02X%02X",AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+0),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+1),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+2),
												 AT24CXX_ReadOneByte(Tag_Stored_Base_Add(j)+3));
			printf("Registered Tags ID %d:%s\n",j,stored_tag_id);
	}
	return 1;
}
MSH_CMD_EXPORT(LIST_ALL_TAGS,list_all_tags);
