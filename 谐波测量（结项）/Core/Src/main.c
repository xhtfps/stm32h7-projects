/***
	*******************************************************************************************************
	*	@file  	main.c
	*	@version V1.0
	*  @date    2021-3-23
	*	@author  反客科技	
	*	@brief   驱动 AT070TN83 显示屏进行显示		
   *******************************************************************************************************
   *  @description
	*
	 
	*
>>>>> 功能说明：
	*
	*	1.驱动AT070TN83进行显示，要注意使用核心板与83进行连接时，要使用同向的FPC排线，
	*	  并且由于83屏幕功耗过大，超出了一般的USB口供电能力，所以建议开启PWM控制背光
	*	2.在刚下载程序时，有时屏幕有轻微抖动属于正常现象，片刻即可恢复正常			
	*
	*******************************************************************************************************
***/


#include "main.h"
#include "led.h"
#include "usart.h"
#include "sdram.h"  
#include "lcd_rgb.h"
#include "lcd_pwm.h"
#include "BSP_4x4KEY.h"
#include "User_UI.h"
#include "lcd_test.h" // 仅用于测试，该文件不是必须，用户移植的时候可以舍弃

#include "AD7606.h"
#include "FFT.h"
#include "math.h"
//画图缓存
__attribute__((section (".RAM_SDRAM"),zero_init)) struct compx FFT_Buff[65536];
__attribute__((section (".RAM_SDRAM"),zero_init)) float FFT_Mag[65536] ;
/********************************************** 函数声明 *******************************************/
void draw_wave();    //绘制波形
void clean_wave();   //清除上一次的波形
void SystemClock_Config(void);		// 时钟初始化
void MPU_Config(void);					// MPU配置
static void TaskA_Handler( void );
static void TaskB_Handler( void );
float xxj=1,xxx=1;
uint8_t bit0=0,bit1=0;
uint32_t data_index=0;
void EXTI_Init(void)
{
    GPIO_InitTypeDef GPIO_Initure;
    
    __HAL_RCC_GPIOB_CLK_ENABLE();               //开启GPIOA时钟
    GPIO_Initure.Pin=GPIO_PIN_0;                //PB0
    GPIO_Initure.Mode=GPIO_MODE_IT_FALLING;      //上升沿触发
    GPIO_Initure.Pull=GPIO_PULLUP;							//下拉
    GPIO_Initure.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);
	HAL_NVIC_SetPriority(EXTI0_IRQn,2,1);       //抢占优先级为2，子优先级为0
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);             //使能中断线0
}
/***************************************************************************************************
*	函 数 名: main
*	入口参数: 无
*	返 回 值: 无
*	函数功能: LTDC驱动屏幕测试
*	说    明: 无
****************************************************************************************************/
uint8_t F_T=1;
uint8_t key_val;
uint8_t mode;
float max,min,vpp;
int main(void)
{
	MPU_Config();				// MPU配置
	SCB_EnableICache();		// 使能ICache
	SCB_EnableDCache();		// 使能DCache
	HAL_Init();					// 初始化HAL库
	SystemClock_Config();	// 配置系统时钟，主频480MHz
	LED_Init();					// 初始化LED引脚
	USART1_Init();				// USART1初始化	
	MX_FMC_Init();				// SDRAM初始化
	MX_LTDC_Init();			// LTDC以及层初始化
    Init_KeyBoard();
    
  //AD7606
	//配日志TIm3的通道2输出PWM信号控制AD7606采样
	AD7606_SetTIMOutPWM2(6400);//Fs=Fc*64*2 （N=512时4周期 N=1024时8周期） 6400
  AD7606_SPI_Init();
	AD7606_SPI_DMA_Init();
 	EXTI_Init();
    
	LCD_SetBackColor(LCD_BLACK); 			//	设置背景色，使用自定义颜色
	LCD_Clear(); 									//	清屏，刷背景色	
  User_UpdataUI(UI_START);
	
	while (1)
	{
        key_val = Read4X4KEY();

        switch( key_val )
        {
            case KEY_A:
                mode=1;
                TaskA_Handler();
              break;
            case KEY_B:
                mode=2;
                TaskB_Handler();
              break;
            default:break;
        }
        LED1_Toggle;
        HAL_Delay(100);
	}
}

static void TaskA_Handler( void )
{
    key_val = KEY_NULL;
    // 更新界面
    User_UpdataUI( UI_TA_MENU );
    // 进入任务A主循环
    float Fs=256000,vpp_max=0,Fre=1000,XLY=0;
    float Fre_J=3000.4,Fre_X=3000.5;//Fre_J:40~70Hz;Fre_X:>70Hz
    float Vpp_J=0.123,Vpp_X=0.123;
    float Fbl=0;
    float  xx1,xx2;
    uint32_t Fre_Point_J=0,Fre_Point_X=0,Fre_Point;
	
	//设置采样率
	Fs = AD7606_Fs_Set(Fs);

    
	LCD_SetTextFont(&CH_Font32);				// 设置3232中文字体,ASCII字体对应为3216
	LCD_SetColor(LCD_WHITE);					//	设置画笔色，使用自定义颜色
    LCD_DisplayText(5, 50,"    基波");
    LCD_DisplayText(5, 50+32,"Fre:         Hz");
    LCD_DisplayText(5, 50+32+32,"V:        V");
    LCD_DisplayText(5, 200,"    谐波");
    LCD_DisplayText(5, 200+32,"Fre:         Hz"); 
    LCD_DisplayText(5, 200+32+32,"V:        V");
    LCD_DisplayText(5, 200+32+32+32,"   含有率");
    LCD_DisplayText(5, 200+32+32+32+32,"       %");
		
	//停止定时器的PWM输出
	HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
	
    while( key_val != KEY_HASH )
    {
        // 读取按键
        key_val = Read4X4KEY();
			
//先测频率：测得基波、谐波频率
		Fs=AD7606_Fs_Set(6400);
			
		MAX_FFT_N=65536;
		Fbl=Fs/MAX_FFT_N;
		AD7606_Samp_Num=65536;
        Fbl = Fs/MAX_FFT_N;
		data_index=0;
        bit1=0;
			
		//开启定时器的PWM输出
		HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
		LCD_DisplayText(0,400,"采集中   ");
		HAL_Delay(12000);
		bit1=0;
		// DispChall();
        //进行FFT
        max=-10;min=10;
        for(uint32_t i=0;i<MAX_FFT_N;i++)
        {
             FFT_Buff[i].real =AD7606_Buffer[i][0]* 10.0 / 32767;
             FFT_Buff[i].imag = 0;
            if(max<FFT_Buff[i].real)max=FFT_Buff[i].real;
            if(min>FFT_Buff[i].real)min=FFT_Buff[i].real;
        }
        vpp=max-min;
        //初始化FFT数据（生成sin,cos表）
        InitTableFFT(MAX_FFT_N);
        //进行FFT
        cfft(FFT_Buff, MAX_FFT_N);
        //对数据进行取模处理
        for(uint32_t i=0;i<MAX_FFT_N;i++)
            FFT_Mag[i]=sqrt(powf(FFT_Buff[i].real,2) + powf(FFT_Buff[i].imag,2))*4.0/MAX_FFT_N/1.1086f;
        //直流
        FFT_Mag[0] = FFT_Mag[0]/4;
        F_T=1;
        clean_wave();
        draw_wave();
        //寻找基波
        Vpp_J=0;
        for(uint32_t i=1;i<MAX_FFT_N/2;i++)//基波
        {
            if(FFT_Mag[i]>Vpp_J)
            {
                Vpp_J=FFT_Mag[i];
                Fre_Point_J=i;
            }
        }
        Fre_J = Fre_Point_J*Fs*1.0f/MAX_FFT_N;//得到频率
      //寻找谐波
        Vpp_X=0;
                for(uint32_t i=1;i<MAX_FFT_N/2;i++)//谐波
        {
            if(FFT_Mag[i]>Vpp_X && FFT_Mag[i]<Vpp_J&&FFT_Mag[i]<0.5&&i*Fs*1.0f/MAX_FFT_N>75)
            {
                Vpp_X=FFT_Mag[i];
                Fre_Point_X=i;
            }
        }
        xx1=Vpp_X;
        Fre_X = Fre_Point_X*Fs*1.0f/MAX_FFT_N;//得到频率

        //更新参数
        LCD_DisplayDecimals( 70, 50+32, Fre_J,  4,3);
        LCD_DisplayDecimals( 70, 200+32, Fre_X,  5,3);
//        LCD_DisplayDecimals( 70, 50+32+32, Vpp_J,  4,3);
        LCD_DisplayDecimals( 70, 200+32+32, Vpp_X*xxx,  4,3);

//********************************************************************2
///基波幅度
	    data_index=0;
        bit1=0;
        HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
//        if(Fre_J<70&&Fre_J>40)
//            Fs=AD7606_Fs_Set(256*Fre_J);
//        else if(Fre_J<40)
//            Fs=AD7606_Fs_Set(40*256);
//        else if(Fre_J>70)
//            Fs=AD7606_Fs_Set(70*256);
        if(Fre_J<700)
            Fs=AD7606_Fs_Set(256*Fre_J);
        else
            Fs=AD7606_Fs_Set(64*Fre_J);
        MAX_FFT_N=256;
        AD7606_Samp_Num=256;
        Fbl = Fs/MAX_FFT_N;
        HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
        HAL_Delay(1000);
        //进行FFT
        for(uint32_t i=0;i<MAX_FFT_N;i++)
        {
             FFT_Buff[i].real =AD7606_Buffer[i][0]* 10.0 / 32767*1.01889182f;
             FFT_Buff[i].imag = 0;
        }
        F_T=0;
        draw_wave();
        //初始化FFT数据（生成sin,cos表）
        InitTableFFT(MAX_FFT_N);
        //进行FFT
        cfft(FFT_Buff, MAX_FFT_N);
        //对数据进行取模处理
        for(uint32_t i=0;i<MAX_FFT_N;i++)
            FFT_Mag[i]=sqrt(powf(FFT_Buff[i].real,2) + powf(FFT_Buff[i].imag,2))*4.0/MAX_FFT_N;
      //直流
        FFT_Mag[0] = FFT_Mag[0]/4;
        
      //寻找基波
        Vpp_J=0;//Vpp_X=0;
        for(uint32_t i=1;i<MAX_FFT_N/2;i++)//基波范围40-70Hz
        {
            if(FFT_Mag[i]>Vpp_J)
            {
                Vpp_J=FFT_Mag[i];
                Fre_Point_J=i;
            }
        }
        //更新参数
        LCD_DisplayDecimals( 70, 50+32+32, Vpp_J*xxj,  4,3);
       // LCD_DisplayDecimals( 70, 200+32+32, Vpp_X,  4,3);
/************************************************************************/
        //计算含有率
        XLY = Vpp_X/Vpp_J;
        LCD_DisplayDecimals( 15, 200+32+32+32+32, XLY*100,  4,1);

        bit1=0;
        data_index=0;
        HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
        LCD_DisplayText(0,400,"采集完成");
		HAL_Delay(1000);
        //系统提示灯
        LED1_Toggle;
        HAL_Delay(100);
    }
    key_val= KEY_NULL;
    User_UpdataUI( UI_START );
}

//画时域波形
static void TaskB_Handler( void )
{
    //变量定义
    key_val = KEY_NULL;
    // 更新界面
    User_UpdataUI( UI_TB_MENU );
    // 进入任务B主循环
    float Fs=256000,vpp_max=0,Fre=1000,XLY=0;
    float Fre_J=3000.4,Fre_X=3000.5;//Fre_J:40~70Hz;Fre_X:>70Hz
    float Vpp_J=0.123,Vpp_X=0.123;
    float Fbl=0;
    float  xx1,xx2;
    uint32_t Fre_Point_J=0,Fre_Point_X=0,Fre_Point;
	Fs = AD7606_Fs_Set(Fs);

    
	LCD_SetTextFont(&CH_Font32);				// 设置3232中文字体,ASCII字体对应为3216
	LCD_SetColor(LCD_WHITE);					//	设置画笔色，使用自定义颜色

    LCD_DisplayText(5, 50,"    基波");
    LCD_DisplayText(5, 50+32,"Fre:         Hz");
    LCD_DisplayText(5, 50+32+32,"V:        V");
    LCD_DisplayText(5, 200,"    谐波");
    LCD_DisplayText(5, 200+32,"Fre:         Hz"); 
    LCD_DisplayText(5, 200+32+32,"V:        V");

	HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
    while( key_val != KEY_HASH )
    {
                // 读取按键
        key_val = Read4X4KEY();
        if(key_val==KEY_D)
        {
            key_val = KEY_NULL;

//先测频率：测得基波、谐波频率
		Fs=AD7606_Fs_Set(6400);
		MAX_FFT_N=65536;
		Fbl=Fs/MAX_FFT_N;
		AD7606_Samp_Num=65536;
        Fbl = Fs/MAX_FFT_N;
		data_index=0;
        bit1=0;
		HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
		LCD_DisplayText(0,400,"校准中   ");
		HAL_Delay(12000);
		bit1=0;
		// DispChall();
        //进行FFT
        max=-10;min=10;
        for(uint32_t i=0;i<MAX_FFT_N;i++)
        {
             FFT_Buff[i].real =AD7606_Buffer[i][0]* 10.0 / 32767;
             FFT_Buff[i].imag = 0;
            if(max<FFT_Buff[i].real)max=FFT_Buff[i].real;
            if(min>FFT_Buff[i].real)min=FFT_Buff[i].real;
        }
        vpp=max-min;
        //初始化FFT数据（生成sin,cos表）
        InitTableFFT(MAX_FFT_N);
        //进行FFT
        cfft(FFT_Buff, MAX_FFT_N);
        //对数据进行取模处理
        for(uint32_t i=0;i<MAX_FFT_N;i++)
            FFT_Mag[i]=sqrt(powf(FFT_Buff[i].real,2) + powf(FFT_Buff[i].imag,2))*4.0/MAX_FFT_N/1.1086f;
        //直流
        FFT_Mag[0] = FFT_Mag[0]/4;
        F_T=1;
        clean_wave();
        draw_wave();
        //寻找基波
        Vpp_J=0;
        for(uint32_t i=1;i<MAX_FFT_N/2;i++)//基波
        {
            if(FFT_Mag[i]>Vpp_J)
            {
                Vpp_J=FFT_Mag[i];
                Fre_Point_J=i;
            }
        }
        Fre_J = Fre_Point_J*Fs*1.0f/MAX_FFT_N;//得到频率
      //寻找谐波
        Vpp_X=0;
        for(uint32_t i=100/Fbl;i<MAX_FFT_N/2;i++)//谐波
        {
            if(FFT_Mag[i]>Vpp_X && FFT_Mag[i]<Vpp_J)
            {
                Vpp_X=FFT_Mag[i];
                Fre_Point_X=i;
            }
        }
        Fre_X = Fre_Point_X*Fs*1.0f/MAX_FFT_N;//得到频率

        //更新参数
        LCD_DisplayDecimals( 70, 50+32, Fre_J,  4,3);
        LCD_DisplayDecimals( 70, 200+32, Fre_X,  5,3);
//        LCD_DisplayDecimals( 70, 50+32+32, Vpp_J,  4,3);
        
//校准谐波        
        xxx=0.5/Vpp_X;
        LCD_DisplayDecimals( 70, 200+32+32, Vpp_X*xxx,  4,3);
//********************************************************************2
///基波幅度
	    data_index=0;
        bit1=0;
        HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
//        if(Fre_J<70&&Fre_J>40)
//            Fs=AD7606_Fs_Set(256*Fre_J);
//        else if(Fre_J<40)
//            Fs=AD7606_Fs_Set(40*256);
//        else if(Fre_J>70)
//            Fs=AD7606_Fs_Set(70*256);
        if(Fre_J<700)
            Fs=AD7606_Fs_Set(256*Fre_J);
        else
            Fs=AD7606_Fs_Set(64*Fre_J);
        MAX_FFT_N=256;
        AD7606_Samp_Num=256;
        Fbl = Fs/MAX_FFT_N;
        HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
        HAL_Delay(1000);
        //进行FFT
        for(uint32_t i=0;i<MAX_FFT_N;i++)
        {
             FFT_Buff[i].real =AD7606_Buffer[i][0]* 10.0 / 32767*1.01889182f;
             FFT_Buff[i].imag = 0;
        }
        F_T=0;
        draw_wave();
        //初始化FFT数据（生成sin,cos表）
        InitTableFFT(MAX_FFT_N);
        //进行FFT
        cfft(FFT_Buff, MAX_FFT_N);
        //对数据进行取模处理
        for(uint32_t i=0;i<MAX_FFT_N;i++)
            FFT_Mag[i]=sqrt(powf(FFT_Buff[i].real,2) + powf(FFT_Buff[i].imag,2))*4.0/MAX_FFT_N;
      //直流
        FFT_Mag[0] = FFT_Mag[0]/4;
        
      //寻找基波
        Vpp_J=0;//Vpp_X=0;
        for(uint32_t i=1;i<MAX_FFT_N/2;i++)//基波范围40-70Hz
        {
            if(FFT_Mag[i]>Vpp_J)
            {
                Vpp_J=FFT_Mag[i];
                Fre_Point_J=i;
            }
        }
//校准基波
        xxj = 5.0/Vpp_J;
        //更新参数
        LCD_DisplayDecimals( 70, 50+32+32, Vpp_J*xxj,  4,3);
       // LCD_DisplayDecimals( 70, 200+32+32, Vpp_X,  4,3);
/************************************************************************/


        bit1=0;
        data_index=0;
		HAL_Delay(1000);
        //系统提示灯
        LED1_Toggle;
        HAL_Delay(100);
        HAL_TIM_PWM_Stop(&TimHandle, TIM_CHANNEL_2);
        LCD_DisplayText(0,400,"校准完成");
        LCD_DisplayDecimals(5, 200+32+32+32,xxj,4,5);
        LCD_DisplayDecimals(5, 200+32+32+32+32,xxx,4,5);
        }
    }
    key_val= KEY_NULL;
    User_UpdataUI( UI_START );
}

void draw_wave()
{
    LCD_SetColor(LCD_WHITE);
    uint32_t x1,x2,y1,y2;
    if(F_T==0)//时域
    {
        for(uint32_t i=0;i<MAX_FFT_N-1;i++)
        {
            x1 = 250+i*1.0/MAX_FFT_N*500;
            x2 = 250+(i+1)*1.0/MAX_FFT_N*500;
            y1 = 250-FFT_Buff[i].real*1.0/10*300/(vpp/10);
            y2 = 250-FFT_Buff[i+1].real*1.0/10*300/(vpp/10);
            LCD_DrawLine(x1,y1,x2,y2);
        }
    }
    else//频域
    {
        for(uint32_t i=0;i<MAX_FFT_N/2-1;i++)
        {
            x1 = 250+i*1.0/MAX_FFT_N*2*500;
            x2 = 250+(i+1)*1.0/MAX_FFT_N*2*500;
            y1 = 430-FFT_Mag[i]/10*300;
            y2 = 430-FFT_Mag[i+1]/10*300;
            LCD_DrawLine(x1,y1,x2,y2);
        }
    }
}
void clean_wave()
{
    LCD_SetColor(LCD_BLACK);
    LCD_FillRect(250,50,500,400);
}


/****************************************************************************************************/
/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 480000000 (CPU Clock)
  *            HCLK(Hz)                       = 240000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  120MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  120MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  120MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  120MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 192
  *            PLL_P                          = 2
  *            PLL_Q                          = 2
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
/****************************************************************************************************/  
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  
  /* 设置LTDC时钟，这里设置为33MHz，即刷新率在60帧左右，过高或者过低都会造成闪烁 */
  /* LCD clock configuration */
  /* PLL3_VCO Input = HSE_VALUE/PLL3M = 1 Mhz */
  /* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 330 Mhz */
  /* PLLLCDCLK = PLL3_VCO Output/PLL3R = 330/10 = 33 Mhz */
  /* LTDC clock frequency = PLLLCDCLK = 33 Mhz */  
   
      
  PeriphClkInitStruct.PLL3.PLL3M = 25;
  PeriphClkInitStruct.PLL3.PLL3N = 330;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 10;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_FMC;               
  PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_D1HCLK;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}


//	配置MPU
void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct;

	HAL_MPU_Disable();		// 先禁止MPU

	MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress      = SDRAM_BANK_ADDR;
	MPU_InitStruct.Size             = MPU_REGION_SIZE_32MB;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable      = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.Number           = MPU_REGION_NUMBER2;
	MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);	// 使能MPU
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != 0x00U)
    {
		SCB_CleanInvalidateDCache();
		HAL_SPI_DeInit(&hspi3);
		HAL_SPI_Init(&hspi3);
		HAL_DMA_DeInit(&hdma_spi3_tx);
		HAL_DMA_Init(&hdma_spi3_tx);
		HAL_DMA_DeInit(&hdma_spi3_rx);
		HAL_DMA_Init(&hdma_spi3_rx);
		
		HAL_SPI_Transmit(&hspi3, (uint8_t*)Spi_Data, AD7606_Channel_Num, 100);
		HAL_SPI_TransmitReceive_DMA(&hspi3, (uint8_t*)Spi_Data, (uint8_t*)&AD7606_Buffer[data_index++][0], AD7606_Channel_Num);
	
		if(data_index==AD7606_Samp_Num/2)//半传输中断
		{
//			NVIC_DisableIRQ(EXTI0_IRQn);//失能中断线0
            bit0=1;
		}
		else if(data_index==AD7606_Samp_Num)//传输完成中断
		{
			data_index=0;
            bit1=1;
        //    DispChall();
		//	DMA2_Flag=1;
		}
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
    }
}

