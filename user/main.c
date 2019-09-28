#include "public.h"

/***********************************用户参数****************************************/

extern uint8_t ucArray_ID[4];            //用于存放RFID模块读到的卡片ID
static uint8_t register_lock = 1;		 //注册操作锁
static uint8_t delete_lock = 1;			 //删除操作锁
static uint8_t psd_lock = 1;             //自定义密码操作锁
static uint8_t reg_err_cnt = 0;          //注册操作密码输入错误计数器
static uint8_t del_err_cnt = 0;          //删除操作密码输入错误计数器
static uint8_t psd_err_cnt = 0;          //自定义密码操作密码输入错误计数器
char register_remain_time[40];           //注册锁定倒计时显示缓存
char delete_remain_time[40];             //删除锁定倒计时显示缓存
char psd_remain_time[40];                //密码自定义锁定倒计时显示缓存
uint8_t flag = 0;                        //标识要显示的模块倒计时

/*****************************线程堆栈，优先级，时间片宏定义************************/

#define THREAD_STACK_SIZE 512
#define THREAD_PRIORITY 21
#define THREAD_TIMESLICE 500

/***********************************创建线程控制块**********************************/

static struct rt_thread Touch_Thread;    //触摸线程控制块
static struct rt_thread Match_Thread;    //卡片匹配线程控制块
static struct rt_thread Door_Open_Thread;//开门线程控制块
static struct rt_thread Err_Psw_Thread;  //密码锁定倒计时线程控制块
static struct rt_thread Err_Reg_Thread;  //注册锁定倒计时线程控制块
static struct rt_thread Err_Del_Thread;  //删除锁定倒计时线程控制块

/***********************************创建信号量***************************************/

rt_sem_t match_thread_lock= RT_NULL;	 //卡片匹配线程锁
rt_sem_t door_open_thread_lock= RT_NULL; //开门线程锁
rt_sem_t err_psd_thread_lock= RT_NULL;   //密码自定义模块密码验证错误倒计时线程锁
rt_sem_t err_reg_thread_lock= RT_NULL;   //注册模块密码验证错误倒计时线程锁
rt_sem_t err_del_thread_lock= RT_NULL;   //删除模块密码验证错误倒计时线程锁


/****************************创建动态定时器控制块变量******************************/
rt_timer_t register_countdown_timer1;    //注册操作锁定倒计时
rt_timer_t delete_countdown_timer2;      //删除操作锁定倒计时
rt_timer_t password_countdown_timer3;    //自定义密码操作锁定倒计时

//注册操作锁定定时器超时函数--注册解锁
void register_timeout1(void *parameter)
{
	register_lock = 1;
}

//删除操作锁定定时器超时函数--删除解锁
void delete_timeout2(void *parameter)
{
	delete_lock = 1;
}

//自定义密码操作锁定定时器超时函数--密码设定解锁
void psd_timeout3(void *parameter)
{
	psd_lock = 1;
}

//根据flag显示倒计时
void down_count(uint8_t flag)
{
	 uint8_t cnt = 0;
	 //清屏
	 LCD_Clear(WHITE);
	 POINT_COLOR = RED;
	 //显示延时5秒后退出
	 while(cnt<5)
	 {   
		  //延时1秒
		  rt_thread_mdelay(1000);
		  cnt++;
		  switch(flag)
		  {
			  case 1: LCD_ShowString(20,120,200,24,24,register_remain_time);break;
			  case 2: LCD_ShowString(20,120,200,24,24,delete_remain_time); break;
			  case 3: LCD_ShowString(20,120,200,24,24,psd_remain_time); break;
			  default:break;
		  }
	 }
	 POINT_COLOR = BLACK;
	 //清屏
	 LCD_Clear(WHITE);
}

//自定义密码锁定倒计时值计算线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char err_psd_thread_stack[512];
/******************自定义密码锁定倒计时值计算线程****************/
static void err_psd_thread(void *parameter)
{
	static rt_tick_t psd_tick = 300000;
	uint8_t min,sec;
	while(1)
	{
		 if(rt_sem_take(err_psd_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
		 {
			while(psd_tick!=0)
			{
				 //如果在显示倒计时的过程中锁被置位将退出倒计时循环
				if(psd_lock==1)
					{
						 psd_tick = 300000;
						 break;
					}
				rt_thread_mdelay(1);
				//减1秒
				psd_tick -=1; 
				//printf("psd_tick:%lu\n",psd_tick);
				//将毫秒转换为分和秒
				min = psd_tick/1000/60;
				sec = psd_tick/1000%60;
				//将结果转换为字符并存放收到显示缓冲区供显示函数调用显示
				sprintf(psd_remain_time,"Please try %dmin  %dsec later!   ",min,sec);
			}
		 }
		 rt_thread_mdelay(10);
	 }
}
						  

//注册锁定倒计时值计算线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char err_reg_thread_stack[512];
/******************注册锁定倒计时值计算线程****************/
static void err_reg_thread(void *parameter)
{
	static rt_tick_t register_tick = 300000;
	uint8_t min,sec;
	while(1)
	{
		 if(rt_sem_take(err_reg_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
		 {
			while(register_tick!=0)
			{
				 //如果在显示倒计时的过程中锁被置位将退出倒计时循环
				if(register_lock==1)
					{
						 register_tick = 300000;
						 break;
					}
				rt_thread_mdelay(1);
				//减1秒
				register_tick -=1; 
				//printf("register_tick:%lu\n",register_tick);
				//将毫秒转换为分和秒
				min = register_tick/1000/60;
				sec = register_tick/1000%60;
				//将结果转换为字符并显示到LCD
				sprintf(register_remain_time,"Please try %dmin  %dsec later!   ",min,sec);
		    }
		 }
		 rt_thread_mdelay(10);
	 }
}


//删除锁定倒计时值计算线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char err_del_thread_stack[512];
/******************删除锁定倒计时值计算线程****************/
static void err_del_thread(void *parameter)
{
	static rt_tick_t delete_tick = 300000;
	uint8_t min,sec;
	while(1)
	{
		 if(rt_sem_take(err_del_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
		 {
			while(delete_tick!=0)
			{
				 //如果在显示倒计时的过程中锁被置位将退出倒计时循环
				if(delete_lock==1)
					{
						 delete_tick = 300000;
						 break;
					}
				rt_thread_mdelay(1);
				//减1秒
				delete_tick -=1; 
				//printf("delete_tick:%lu\n",delete_tick);
				//将毫秒转换为分和秒
				min = delete_tick/1000/60;
				sec = delete_tick/1000%60;
				//将结果转换为字符并显示到LCD
				sprintf(delete_remain_time,"Please try %dmin  %dsec later!   ",min,sec);
		    }
		 }
		 rt_thread_mdelay(10);
	 }
}

//开门线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char door_open_thread_stack[512];
/******************开门线程****************/
static void door_open_thread(void *parameter)
{
	while(1)
	{
		if(rt_sem_take(door_open_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
		{
			//开始
			HAL_TIM_PWM_Start(&TIM3_Handler,TIM_CHANNEL_2);
			//开门
			TIM_SetTIM3Compare2(4000);
			rt_thread_mdelay(3500);
			//暂停
			HAL_TIM_PWM_Stop(&TIM3_Handler,TIM_CHANNEL_2);
			rt_thread_mdelay(2500);
			//关门
			HAL_TIM_PWM_Start(&TIM3_Handler,TIM_CHANNEL_2);
			TIM_SetTIM3Compare2(3000);
			rt_thread_mdelay(4500);
			//停止
			HAL_TIM_PWM_Stop(&TIM3_Handler,TIM_CHANNEL_2);
		}
		rt_thread_mdelay(10);
	}
}

//tag匹配线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char match_thread_stack[1024];
/******************tag匹配线程****************/
static void match_thread(void *parameter)
{
	while(1)
	{
		if(rt_sem_take(match_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
		{
			if(Read_ID()==MI_OK)
			{
				if(tag_match(ucArray_ID)==1)
				{
					//释放信号量开门
					rt_sem_release(door_open_thread_lock);
					POINT_COLOR = RED;
					beeponce(35);
					LCD_ShowString(20, 200,240,240,16 ,"                        ");
					LCD_ShowString(70,120,200,16,24,"Welcome!");
					LCD_ShowString(70,150,100,16,24,"~      ~");
					LCD_ShowString(70,170,100,16,24,"   __   ");
					rt_thread_mdelay(2500);
					POINT_COLOR = BLACK;
					LCD_ShowString(70,120,200,16,24,"         ");
					LCD_ShowString(70,150,100,16,24,"         ");
					LCD_ShowString(70,170,100,16,24,"         ");
					LCD_ShowString( 0, 80,240,240,16 ,"The Card ID is:               ");
				}
				else
					beeponce(150);
			}
			rt_sem_release(match_thread_lock);
			rt_thread_mdelay(5);
		}
	}
}
	
//触摸线程堆栈
ALIGN(RT_ALIGN_SIZE)
static char touch_thread_stack[1024];
/******************触摸线程****************/
static void touch_thread(void *parameter)
{
	
	while(1)
   {
		//触摸屏扫描
		tp_dev.scan(0); 	
        //判断触摸屏被按下		  
		if(tp_dev.sta&TP_PRES_DOWN)			
		{	
			//检测触摸范围是否在液晶显示尺寸之内
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height) 
			{	
				
				// 一. 判断Register是否被按下
				if(Register)
				{
					//触摸提示音
					beeponce(10);
					//挂起match_thread线程
					if(rt_sem_take(match_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
					{
						//禁止RTC中断
						HAL_NVIC_DisableIRQ(RTC_IRQn);
						//锁定标志位为1时，可以进入注册操作
						if(register_lock)
						{
								//注册新卡片前先验证密码
								switch(Match_psw())
								{
									case 1:
										  {
											  //密码输入正确时将错误计数器清零
											  reg_err_cnt = 0;
											  //注册新卡片
											  Register_New_Tag();
											  break;
										  }
									case 2:
										  {
											  //记录错误输入次数
											   reg_err_cnt++;
											   //大于三次，进入注册功能进入5分钟锁定状态
											   if(reg_err_cnt>2)
											   {
												    //错误计数器清零
													reg_err_cnt = 0;
												    //锁定标志位清零
													register_lock = 0;
												    //释放信号量，启动倒计时计数值获取线程
													rt_sem_release(err_reg_thread_lock);
												    //启动定时器
												    rt_timer_start(register_countdown_timer1);
											   } 
											   POINT_COLOR = RED;
											   LCD_ShowString(0,46,200,24,24,"  Wrong Password!"); 
											   POINT_COLOR = BLACK;	
											   rt_thread_mdelay(2000);
											   break;
										  }
									case 0:
										  {
											   break;
										  }
									default: break;
								 }
						}
						//锁定标志位为0时，显示等待解锁时间
						else
						{
							down_count(1);
						}
						HAL_NVIC_EnableIRQ(RTC_IRQn);	
						//恢复主界面
						security_door_window(); 
						//释放信号量，唤醒match_thread线程
						rt_sem_release(match_thread_lock);
				  }
				   goto loop;
			  }					
				//二. 判断Delete是否被按下
				if(Delete)
				{
					//触摸提示音
					beeponce(10);
					//挂起match_thread线程
					if(rt_sem_take(match_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
					{
						//禁止RTC中断
						HAL_NVIC_DisableIRQ(RTC_IRQn);
						//锁定标志位为1时，可以进入注册操作
						if(delete_lock)
						{
								//注册新卡片前先验证密码
								switch(Match_psw())
								{
									case 1:
										  {
									         //错误计数器清零
											  del_err_cnt = 0;
											  Delete_registered_Tag();
											  break;
										  }
									case 2:
										  {
											   //记录错误输入次数
											   del_err_cnt++;
											   //大于三次，进入注册功能进入5分钟锁定状态
											   if(del_err_cnt>2)
											   {
												    //错误计数器清零
													del_err_cnt = 0;
												    //锁定标志位清零
													delete_lock = 0;
												     //释放信号量，启动倒计时计数值获取线程
													rt_sem_release(err_del_thread_lock);
												    rt_timer_start(delete_countdown_timer2);
											   } 
											   POINT_COLOR = RED;
											   LCD_ShowString(0,46,200,24,24,"  Wrong Password!"); 
											   POINT_COLOR = BLACK;	
											   rt_thread_mdelay(2000);
											   break;
										  }
									case 0:
										  {
											   break;
										  }
									default: break;
								 }
							 }
						//锁定标志位为0时，显示等待解锁时间
						else
						{
							 down_count(2);
						}
						HAL_NVIC_EnableIRQ(RTC_IRQn);
						//恢复主界面
						security_door_window(); 
						//释放信号量，唤醒match_thread线程
						rt_sem_release(match_thread_lock);
				  }
				   goto loop;
				}
				//三. 检测密码设置键是否被按下
				if(Set_Psw)
				{
					//触摸提示音
					beeponce(10);
					//挂起match_thread线程
					if(rt_sem_take(match_thread_lock,RT_WAITING_FOREVER)==RT_EOK)
					{
						//禁止RTC中断
						HAL_NVIC_DisableIRQ(RTC_IRQn);
						//锁定标志位为1时，可以进入注册操作
						if(psd_lock)
						{
								//注册新卡片前先验证密码
								switch(Match_psw())
								{
									case 1:
											{
											    //错误计数器清零
												psd_err_cnt = 0;
												//验证成功进入密码设置
												if(password_setting()==1)
												{
													POINT_COLOR = RED;
													LCD_ShowString(0,46,200,24,24,"Password Set OK!");
													LCD_ShowString(0,70,200,24,24,"                ");
													POINT_COLOR = BLACK;
													rt_thread_mdelay(2000);
												}
												//返回，不进行任何操作
												else
												{
													;
												}
												break;
											}
									case 2:
											{
												//记录错误输入次数
											   psd_err_cnt++;
											   //大于三次，进入注册功能进入5分钟锁定状态
											   if(psd_err_cnt>2)
											   {
												    //错误计数器清零
													psd_err_cnt = 0;
												    //锁定标志位清零
													psd_lock = 0;
												     //释放信号量，启动倒计时计数值获取线程
													rt_sem_release(err_psd_thread_lock);
												    //启动定时器
												    rt_timer_start(password_countdown_timer3);
											   } 
												//密码验证失败，显示错误信息，然后退出
												POINT_COLOR = RED;
												LCD_ShowString(0,46,200,24,24," Wrong  Password!");
												LCD_ShowString(0,70,200,24,24,"                ");
												POINT_COLOR = BLACK;
												rt_thread_mdelay(2000);
												break;
											}
									case 0:
											{
												//用户取消密码输入验证，直接退出
												break;
											}
									default:break;
								}
						}
						//锁定标志位为0时，显示等待解锁时间
						else
						{
							down_count(3);
						}
						HAL_NVIC_EnableIRQ(RTC_IRQn);	
						//恢复主界面
						security_door_window();
						//释放信号量，唤醒match_thread线程
						rt_sem_release(match_thread_lock);
					}
				}
				loop:
			    //消除触摸抖动  
				while(tp_dev.sta&TP_PRES_DOWN)	
				{
					tp_dev.scan(0); 
				}
			}
		}
		rt_thread_mdelay(5);
	}
}

int Door_Security_Demo(void)
{
	rt_err_t result = 0;
	//创建定时器1线程--register锁定
	register_countdown_timer1 = rt_timer_create("register_countdown_timer1",register_timeout1,RT_NULL,300000,RT_TIMER_CTRL_SET_ONESHOT|RT_TIMER_FLAG_SOFT_TIMER);
	//启动定时器1线程
	if(register_countdown_timer1 != RT_NULL)
		rt_kprintf("register_countdown_timer1 created!\n");
		
	
	//创建定时器2线程--delete锁定
	delete_countdown_timer2 = rt_timer_create("delete_countdown_timer2",delete_timeout2,RT_NULL,300000,RT_TIMER_CTRL_SET_ONESHOT|RT_TIMER_FLAG_SOFT_TIMER);
	//启动定时器2线程
	if(delete_countdown_timer2 != RT_NULL)
		rt_kprintf("delete_countdown_timer2 created!\n");
		//
	
	//创建定时器3线程--psd锁定
	password_countdown_timer3 = rt_timer_create("password_countdown_timer3",psd_timeout3,RT_NULL,300000,RT_TIMER_CTRL_SET_ONESHOT|RT_TIMER_FLAG_SOFT_TIMER);
	//启动定时器2线程
	if(password_countdown_timer3 != RT_NULL)
		rt_kprintf("password_countdown_timer3 created!\n");

	
	//创建卡片匹配线程锁信号量
	match_thread_lock = rt_sem_create("match_thread_lock",1,RT_IPC_FLAG_FIFO);
	if(match_thread_lock!=RT_NULL)
	{
		rt_kprintf("match_thread_lock created!\n");
	}
	//创建开门线程锁信号量
	door_open_thread_lock = rt_sem_create("door_open_thread_lock",0,RT_IPC_FLAG_FIFO);
	if(door_open_thread_lock!=RT_NULL)
	{
		rt_kprintf("door_open_thread_lock created!\n");
	}
	
	//创建密码模块倒计时值计算线程锁信号量
	err_psd_thread_lock = rt_sem_create("err_psd_thread_lock",0,RT_IPC_FLAG_FIFO);
	if(err_psd_thread_lock!=RT_NULL)
	{
		rt_kprintf("err_psd_thread_lock created!\n");
	}
	
	//创建注册模块倒计时值计算线程锁信号量
	err_reg_thread_lock = rt_sem_create("err_reg_thread_lock",0,RT_IPC_FLAG_FIFO);
	if(err_reg_thread_lock!=RT_NULL)
	{
		rt_kprintf("err_reg_thread_lock created!\n");
	}
	
	//创建删除模块倒计时值计算线程锁信号量
	err_del_thread_lock = rt_sem_create("err_del_thread_lock",0,RT_IPC_FLAG_FIFO);
	if(err_reg_thread_lock!=RT_NULL)
	{
		rt_kprintf("err_del_thread_lock created!\n");
	}
	
	//tag匹配线程
    result = rt_thread_init( &Match_Thread,
							"match_thread",
							 match_thread,
							 RT_NULL,
							 &match_thread_stack[0],
							 sizeof(match_thread_stack),
							 THREAD_PRIORITY+1,
							 THREAD_TIMESLICE  	
							);
	//启动tag匹配线程
	if(result==RT_EOK)
	{
		rt_thread_startup(&Match_Thread);
	}
	
	//开门线程
    result = rt_thread_init( &Door_Open_Thread,
							"Door_Open_Thread",
							 door_open_thread,
							 RT_NULL,
							 &door_open_thread_stack[0],
							 sizeof(door_open_thread_stack),
							 THREAD_PRIORITY+2,
							 THREAD_TIMESLICE  	
							);
	//启动开门线程
	if(result==RT_EOK)
	{
		rt_thread_startup(&Door_Open_Thread);
	}
	
	//创建触摸线程
    result = rt_thread_init( &Touch_Thread,
							"touch_thread",
							 touch_thread,
							 RT_NULL,
							 &touch_thread_stack[0],
							 sizeof(touch_thread_stack),
							 THREAD_PRIORITY-18,//该优先级低于Match_Thread线程触摸会明显迟钝
							 THREAD_TIMESLICE  	
							);
	//启动触摸线程 
	if(result==RT_EOK)
	{
	  rt_thread_startup(&Touch_Thread);
	}
	
	//创建密码模块倒计时值计算线程
    result = rt_thread_init( &Err_Psw_Thread,
							"Err_Psw_Thread",
							 err_psd_thread,
							 RT_NULL,
							 &err_psd_thread_stack[0],
							 sizeof(err_psd_thread_stack),
							 THREAD_PRIORITY-15, 
							 THREAD_TIMESLICE  	
							);
	//启动密码模块倒计时值计算线程
	if(result==RT_EOK)
	{
	  rt_thread_startup(&Err_Psw_Thread);
	}
	
	//创建注册模块倒计时值计算线程
    result = rt_thread_init( &Err_Reg_Thread,
							"Err_Reg_Thread",
							 err_reg_thread,
							 RT_NULL,
							 &err_reg_thread_stack[0],
							 sizeof(err_reg_thread_stack),
							 THREAD_PRIORITY-16, 
							 THREAD_TIMESLICE  	
							);
	//启动注册模块倒计时值计算线程
	if(result==RT_EOK)
	{
	  rt_thread_startup(&Err_Reg_Thread);
	}
	
	//创建删除模块倒计时值计算线程
    result = rt_thread_init( &Err_Del_Thread,
							"Err_Del_Thread",
							 err_del_thread,
							 RT_NULL,
							 &err_del_thread_stack[0],
							 sizeof(err_del_thread_stack),
							 THREAD_PRIORITY-17, 
							 THREAD_TIMESLICE  	
							);
	//启动删除模块倒计时值计算线程
	if(result==RT_EOK)
	{
	  rt_thread_startup(&Err_Del_Thread);
	}
	return 0;
}



//主函数
int main ( void )
{  
	psw_check();
	Door_Security_Demo();
}





/****************************END OF FILE**********************/


