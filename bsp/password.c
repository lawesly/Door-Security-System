#include "public.h"

/* 函数名：psw_check
 * 描述  ：开机时检测密码是否被修改，标志:0xaa
 * 输入  ：无
 * 返回  : 无
 * 调用  ：初始化时调用              
*/
void psw_check(void)
{
    uint8_t tem = 0;
	tem = AT24CXX_ReadOneByte(PSW_FLAG);
	//如果之前没设置密码，则进入密码设置环节
	if(tem!=0xaa)
	{
		//清屏
		LCD_Clear(WHITE);
		//显示密码设置GUI
		Psd_setting_GUI(SET_PSW);
		//设置密码
		if(password_setting())
		{
			//密码设置完成设置标志位
			AT24CXX_WriteOneByte(PSW_FLAG,0xaa);
			POINT_COLOR = RED;
			LCD_ShowString(0,46,200,24,24," Password Set OK! ");
			POINT_COLOR = BLACK;
			rt_thread_mdelay(3500);
			LCD_Clear(WHITE);
		}
		else
		{
			POINT_COLOR = RED;
			LCD_ShowString(0,46,200,24,24,"Password Setting cancelled!");
			POINT_COLOR = BLACK;
			rt_thread_mdelay(3500);
			LCD_Clear(WHITE);
		}
	}
}


/* 函数名：password_setting
 * 描述  ：密码设置
 * 输入  ：无
 * 返回  : 无
 * 调用  ：psw_check()函数调用              
*/
uint8_t password_setting(void)
{
	char psw[6];
	uint8_t i;
	//获取用户输入的密码
    if(Get_User_Input_psw(psw)==1)
	{
		//printf("psw:%s\n",psw);
		//将密码写入到指定EEPROM地址
		for(i=0;i<6;i++)
		{
		 AT24CXX_WriteOneByte(PSW_ADD_BASE+i,psw[i]);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}


/* 函数名：Get_Touch_KeyValue
 * 描述  ：触摸按键值获取
 * 输入  ：无
 * 返回  : 无
 * 调用  ：password_setting()函数调用              
*/
uint8_t Get_Touch_KeyValue(void)
{
	    //触摸屏扫描
		tp_dev.scan(0); 	
        //判断触摸屏被按下		  
		if(tp_dev.sta&TP_PRES_DOWN)			
		{	
			//检测触摸范围是否在液晶显示尺寸之内
		 	if((tp_dev.x[0]<lcddev.width)&&(tp_dev.y[0]<lcddev.height)) 
			{	
				//消除触摸抖动  
				while(tp_dev.sta&TP_PRES_DOWN)	
				{
					tp_dev.scan(0); 
				}
				//触摸提示音
				beeponce(10);
				//判断One是否被按下
				if(One)
				{
					return 1;
				}	
				//判断Two是否被按下
				if(Two)
				{
					return 2;
				}
				//判断Three是否被按下
				if(Three)
				{
					return 3;
				}
				//判断Four是否被按下
				if(Four)
				{
					return 4;
				}
			    //判断Five是否被按下
				if(Five)
				{
					return 5;
				}
			    //判断Six是否被按下
				if(Six)
				{
					return 6;
				}
			    //判断Seven是否被按下
				if(Seven)
				{
					return 7;
				}
			    //判断Eight是否被按下
				if(Eight)
				{
					return 8;
				}
			    //判断Nine是否被按下
				if(Nine)
				{
					return 9;
				}
			    //判断D/B是否被按下
				if(DB)
				{
					 
					return 67;
				}
			    //判断Zero是否被按下
				if(Zero)
				{
					return 0;
				}
			    //判断OK是否被按下
				if(OK)
				{
					 
					return 69;
				}
			}
		}
	return 'a';	
}



/* 函数名：Get_User_Input_psw
 * 描述  ：获取用户输入的密码
 * 输入  ：无
*  返回  : uint8_t psw[6]
		   1:成功获取用户密码
		   0:密码设置取消返回
 * 调用  ：卡片删除和注册函数调用              
*/
uint8_t Get_User_Input_psw(char psw[6])
{
	uint8_t tem,i = 0;
	while(1)
	{
		loop:
		//获取触摸按键内容
		tem = Get_Touch_KeyValue();
		if(tem!='a')
		{
			//一. 如果输入的是数值，则将其转换为字符，然后显示到LCD
			if((tem>=0)&&(tem<=9))
			{
				psw[i++] = tem+'0';
				//密码位数超过6位重新输入
				if(i>6)      
				{  
				    i = 0;
					//清除之前的输入
				    for(uint8_t k=0;k<=6;k++)
					     { psw[k] = ' ';}
				}			
				//显示到LCD
                LCD_ShowString(0,70,200,24,24,psw);	
			}
			//二. 输入的是返回或删除，显示B/D
			if(tem==67)
			{
				//没有输入任何数值时，按B/D键退出密码设定操作
				if(i==0)
				{		
                    LCD_ShowString(0,70,200,24,24,"                   ");							
					return 0;
				}
				else
				{
					//没有输入的情况下按B/D返回
//					if(i==0)
//					{
//						LCD_ShowString(0,70,200,24,24,"                  ");	
//					    return 0;
//					}
					//将最后一次输入的数删除
					psw[(--i)] =' ';
					//显示到LCD
					LCD_ShowString(0,70,200,24,24,psw);		
				}
			}
			//三.输入的是确认键，则返回结果
			if(tem==69)
			{
				//没有输入的情况下，按E重新输入
				if(i==0)
				{goto loop;}
				//获取用户密码后返回
				//printf("psw:%s\n",psw);
				LCD_ShowString(0,70,200,24,24,"          ");	
				return 1;
			}
		}
	}
}

/* 函数名：Match_psw
 * 描述  ：将用户的密码与EEPROM中保存的密码进行匹配
 * 输入  ：无
*  返回  : 1:匹配成功
		   2:匹配失败返回2
		   0:用户取消密码输入验证
 * 调用  ：卡片删除和注册函数调用              
*/
uint8_t Match_psw(void)
{
	uint8_t i = 0;
	char eeprpm_psw[7];
	char user_psw[7];
	//清屏
	LCD_Clear(WHITE);
	//显示密码设置GUI
	Psd_setting_GUI(MATCH_PSW);
	//将EEPROM中保存的密码读出来
	for(i=0;i<6;i++)
	eeprpm_psw[i]=AT24CXX_ReadOneByte(PSW_ADD_BASE+i);
	eeprpm_psw[6]= '\0';
	//获取用户输入的密码
	if(Get_User_Input_psw(user_psw)==1)
	{
		//添加结束符
		user_psw[6]= '\0';
		//开始匹配
		if(rt_strcmp(eeprpm_psw,user_psw)==0)
			return 1; //匹配成功返回1
		else
		{
			
			return 2; //匹配失败返回2
		}
	}
	else
	{
		 return 0;   //用户取消密码输入验证
	}
		
}


/* 函数名：Psd_setting_GUI
 * 描述  ：显示密码设定界面
 * 输入  ：无
 * 返回  : 无
 * 调用  ：psd_check()函数调用              
*/
void Psd_setting_GUI(uint8_t mode)
{
  POINT_COLOR = RED;
  if(mode==SET_PSW)
  {
    LCD_ShowString(20,0,200,24,24,"Setting  Password");//显示标题	
	LCD_ShowString(0,46,200,24,24,"New Password:");
  }
  else
  {
    LCD_ShowString(20,0,200,24,24,"Verify   Password");
	LCD_ShowString(0,46,200,24,24,"Password:");  
  }
  POINT_COLOR = BLACK;
  LCD_DrawLine(0, 24, 240, 24);//第一条横线
  LCD_ShowString(0,70,200,24,24,"            ");
  LCD_DrawLine(1, 160, 240, 160);//画第yi条横线
  LCD_DrawLine(0, 200, 240, 200);//画第er条横线
  LCD_DrawLine(0, 240, 240, 240);//画第san条横线
  LCD_DrawLine(0, 280, 240, 280);//画第si条横线
  LCD_DrawLine(80, 160, 80, 320);//画第1条竖线	
  LCD_DrawLine(160, 160, 160, 320);	//画第2条竖线
  LCD_ShowString(38,178-8,200,24,24,"1");
  LCD_ShowString(116,178-8,200,24,24,"2");	
  LCD_ShowString(199,178-8,200,24,24,"3");
  LCD_ShowString(38,221-8,200,24,24,"4");
  LCD_ShowString(116,221-8,200,24,24,"5");
  LCD_ShowString(199,221-8,200,24,24,"6");	
  LCD_ShowString(38,257-8,200,24,24,"7");
  LCD_ShowString(116,257-8,200,24,24,"8");
  LCD_ShowString(199,257-8,200,24,24,"9");
  LCD_ShowString(25,297-8,200,24,24,"B/D");	
  LCD_ShowString(113,297-8,200,24,24,"0");
  LCD_ShowString(199,297-8,200,24,24,"E");	
}

/* 函数名：Get_User_Psd
 * 描述  ：通过PC串口发送msh命令查看用户密码
 * 输入  ：无
 * 返回  : 无
 * 调用  ：MSH调用              
*/
uint8_t Get_User_Psd(void)
{
	uint8_t i = 0;
	char str[7];
	//将EEPROM中保存的密码读出来
	for(i=0;i<6;i++)
	str[i]=AT24CXX_ReadOneByte(PSW_ADD_BASE+i);
	str[6]= '\0';
	printf("User Password is:%s\n",str);
	return 1;
}
MSH_CMD_EXPORT(Get_User_Psd,Get_User_Psd);
