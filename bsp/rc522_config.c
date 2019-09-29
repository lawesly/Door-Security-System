#include "rc522_config.h"
#include "stm32f1xx_hal.h"

static void RC522_SPI_Config( void );

/**
  * @brief  RC522_Init
  * @param  无
  * @retval 无
  */
void RC522_Init ( void )
{
	RC522_SPI_Config();	
	RC522_Reset_Disable();	
	RC522_CS_Disable();
	PcdReset();
    /*设置工作方式*/
	M500PcdConfigISOType( 'A' );
	printf("RC522 Initilized!\n");
}

/**
  * @brief  R522 SPI配置
  * @param  无
  * @retval 无
  */
static void RC522_SPI_Config ( void )
{
    GPIO_InitTypeDef GPIO_Initure;
	//__HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();       //使能GPIOB时钟
    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    
    //PA4,5,7
    GPIO_Initure.Pin=GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;          //一定要配置为推挽输出，复用推挽输出会导致SPI无法工作
    GPIO_Initure.Pull=GPIO_PULLUP;                  //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;        //快速            
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	 //PA6
	GPIO_Initure.Pin=GPIO_PIN_6;
    GPIO_Initure.Mode=GPIO_MODE_INPUT;              //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;        //快速            
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
	
	//PB12
    GPIO_Initure.Pin=GPIO_PIN_12;
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;          //一定要配置为推挽输出，复用推挽输出会导致SPI无法工作
    GPIO_Initure.Pull=GPIO_PULLUP;                  //上拉
    GPIO_Initure.Speed=GPIO_SPEED_FREQ_HIGH;        //快速            
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
	
}


