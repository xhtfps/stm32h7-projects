#include "AD7606.h"

/* 8路同步采集，每次采集16字节数据，防止DMA突发方式1KB边界问题，即每次采集不要有跨边界的情况 */

uint32_t AD7606_Samp_Num = 65536; 
//#pragma location = ".RAM_D3"
//int16_t AD7606_Buffer[AD7606_Samp_Num][AD7606_Channel_Num]  __attribute__((section(".ARM.__at_0x24000000"))); //AD7606单次采集缓冲区
//#pragma location = ".RAM_D3"
uint16_t Spi_Data[AD7606_Channel_Num]  __attribute__((section(".ARM.__at_0x24000000")));

__attribute__((section (".RAM_SDRAM"),zero_init)) int16_t AD7606_Buffer[65536][AD7606_Channel_Num];
float AD7606_Fs=6400.0;

TIM_HandleTypeDef TimHandle = {0};
SPI_HandleTypeDef hspi3;

#define BUSY_IS_LOW()				(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET)

void DispOnce()
{
	uint16_t i;
	for(i=0;i<AD7606_Channel_Num;i++)
	{
		printf("ch[%d] :%f\r\n",i%8,AD7606_Buffer[0][i]* 10.0 / 32767);
	}
}

void DispChall()
{
	uint16_t i;
	printf("------------\r\n");
	for(i=0;i<AD7606_Samp_Num;i++)
	{
		printf("%f\r\n",AD7606_Buffer[i][0]* 10.0 / 32767);
	}
}

uint16_t spiRead1(void)
{

	int16_t read;
	/* 读取接收到的数据 */
   HAL_SPI_TransmitReceive(&hspi3, (uint8_t*)Spi_Data, (uint8_t*)&read, 1, 1000);

	/* 返回读到的数据 */
	return read;
//#endif
}

void AD7606_Scan(void) 		/* 此函数代码按照时序编写 */
{
	uint8_t i;			
	/* BUSY = 0 时.ad7606处于空闲状态ad转换结束 */	
	if (BUSY_IS_LOW())	   
  {
//		AD_CS_0(); /* SPI片选 = 0 */
	
		for (i = 0; i < AD7606_Channel_Num; i++)
		{
			AD7606_Buffer[0][i] = spiRead1();
		}
	
//		AD_CS_1(); /* SPI片选 = 1 */

//		AD7606_StartConv();	/* 给开始信号 */	
	}
}

void AD7606_SPI_Init(void)
{
	uint16_t i;
	GPIO_InitTypeDef  GPIO_InitStruct;
	
	for(i=0;i<AD7606_BUFSIZE;i++)
	{
		Spi_Data[i]=0;
	}
	
	/* SPI和GPIP时钟 */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_SPI3_CLK_ENABLE();
	
	/* SPI SCK */
	GPIO_InitStruct.Pin       = GPIO_PIN_3;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* SPI MISO */
	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* SPI MOSI */
	GPIO_InitStruct.Pin = GPIO_PIN_2;//冲突
	GPIO_InitStruct.Alternate = GPIO_AF7_SPI3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_1;//冲突
	GPIO_InitStruct.Alternate = GPIO_AF7_SPI3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/* BUSY */
//	GPIO_InitStruct.Pin       = GPIO_PIN_0;
//	GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
//	GPIO_InitStruct.Pull      = GPIO_NOPULL;
//	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
//	GPIO_InitStruct.Alternate = 0;
//	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/* 设置SPI参数 */
	hspi3.Instance               = SPI3;                  			/* 例化SPI */
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;     /* 设置波特率 不能太高 */
	hspi3.Init.Direction         = SPI_DIRECTION_2LINES_RXONLY;   			/* 全双工 */
	hspi3.Init.CLKPhase          = SPI_PHASE_2EDGE;             /* 配置时钟相位 */
	hspi3.Init.CLKPolarity       = SPI_POLARITY_HIGH;           /* 配置时钟极性 */
	hspi3.Init.DataSize          = SPI_DATASIZE_16BIT;      		/* 设置数据宽度 */
	hspi3.Init.FirstBit          = SPI_FIRSTBIT_MSB;       			/* 数据传输先传高位 */
	hspi3.Init.TIMode            = SPI_TIMODE_DISABLE;    		  /* 禁止TI模式  */
	hspi3.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;  /* 禁止CRC */
	hspi3.Init.CRCPolynomial     = 7;                       		/* 禁止CRC后，此位无效 */
	hspi3.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;    		  /* 禁止CRC后，此位无效 */
	hspi3.Init.NSS               = SPI_NSS_SOFT;               	/* 使用软件方式管理片选引脚 */
	hspi3.Init.FifoThreshold     = SPI_FIFO_THRESHOLD_08DATA;  	/* 设置FIFO大小是一个数据项 */
	hspi3.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;       /* 禁止脉冲输出 */
	hspi3.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE; /* 禁止SPI后，SPI相关引脚保持当前状态 */  
	hspi3.Init.Mode 					  	= SPI_MODE_MASTER;            /* SPI工作在主控模式 */

	/* 复位配置 */
	if(HAL_SPI_DeInit(&hspi3) != HAL_OK)
	{
	//	Error_Handler();
	}

	/* 初始化配置 */
	if(HAL_SPI_Init(&hspi3) != HAL_OK)
	{
		//Error_Handler();
	}
	
}




//TIM3(84M) CH2 PB5                TIM4(84M) CH1 PD12
void AD7606_SetTIMOutPWM2(uint32_t Fs)
{
		uint16_t usPeriod;
		uint16_t usPrescaler;
		uint32_t pulse;
	
		GPIO_InitTypeDef GPIO_InitStruct;
		TIM_OC_InitTypeDef sConfig = {0};	
		
		/* 配置时钟 */
		__HAL_RCC_GPIOB_CLK_ENABLE(); 	//__HAL_RCC_GPIOD_CLK_ENABLE PWM端口复用   
		__HAL_RCC_TIM3_CLK_ENABLE();		//__HAL_RCC_TIM4_CLK_ENABLE
//		__HAL_RCC_DMA1_CLK_ENABLE();	//使能DMA1时钟
	
		/* 配置引脚 */
		GPIO_InitStruct.Pin 			= GPIO_PIN_5;							//GPIO_PIN_12
		GPIO_InitStruct.Mode 			= GPIO_MODE_AF_PP;       	//复用功能
		GPIO_InitStruct.Speed 		= GPIO_SPEED_FREQ_MEDIUM;	//速度100MHz
		GPIO_InitStruct.Pull			= GPIO_PULLUP;        		//上拉
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;					//GPIO_AF2_TIM4
		HAL_GPIO_Init(GPIOB,&GPIO_InitStruct);   						//GPIOD 
	/*-----------------------------------------------------------------------
		system.c 文件中 void SystemClock_Config(void) 函数对时钟的配置如下: 

        System Clock source       = PLL (HSE)
        SYSCLK(Hz)                = 400000000 (CPU Clock)
        HCLK(Hz)                  = 200000000 (AXI and AHBs Clock)
        AHB Prescaler             = 2
        D1 APB3 Prescaler         = 2 (APB3 Clock  100MHz)
        D2 APB1 Prescaler         = 2 (APB1 Clock  100MHz)
        D2 APB2 Prescaler         = 2 (APB2 Clock  100MHz)
        D3 APB4 Prescaler         = 2 (APB4 Clock  100MHz)

        因为APB1 prescaler != 1, 所以 APB1上的TIMxCLK = APB1 x 2 = 200MHz;
        因为APB2 prescaler != 1, 所以 APB2上的TIMxCLK = APB2 x 2 = 200MHz;
        APB4上面的TIMxCLK没有分频，所以就是100MHz;

        APB1 定时器有 TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14，LPTIM1
        APB2 定时器有 TIM1, TIM8 , TIM15, TIM16，TIM17

        APB4 定时器有 LPTIM2，LPTIM3，LPTIM4，LPTIM5

	----------------------------------------------------------------------- */
		if (Fs < 100)
		{
			usPrescaler = 10000 - 1;						/* 分频比 = 10000 */
			usPeriod =  (SystemCoreClock/2 / 10000) / Fs  - 1;	/* 自动重装的值， usPeriod最小值168, 单位59us */
			pulse = usPeriod;                               /* 设置低电平时间59us，注意usPeriod已经进行了减1操作 */
		} 
		else if (Fs < 3000)
		{
			usPrescaler = 100 - 1;							/* 分频比 = 100 */
			usPeriod =  (SystemCoreClock/2 / 100) / Fs  - 1;	/* 自动重装的值， usPeriod最小值560，单位595ns */
			pulse = usPeriod-1;                           	/* 设置低电平时间1.19us，注意usPeriod已经进行了减1操作 */
		}
		else	/* 大于4K的频率，无需分频 */
		{
			usPrescaler = 0;								/* 分频比 = 1 */
			usPeriod = SystemCoreClock/2 / Fs - 1;				/* 自动重装的值， usPeriod最小值840，单位5.95ns */
			pulse = usPeriod - 199;              			/* 设置低电平时间1.19us，注意usPeriod已经进行了减1操作 */
		}

		printf( "Pre:%d Psc:%d Fre:%f\r\n",usPrescaler+1,usPeriod+1,SystemCoreClock/2.0/(usPrescaler+1)/(usPeriod + 1) );
		//PWM频率 = TIMxCLK / usPrescaler + 1）/usPeriod + 1）			
		TimHandle.Instance = TIM3;//TIM4
		TimHandle.Init.Prescaler         = usPrescaler;         /* 用于设置定时器分频 Timer clock = sysclock /(TIM_Prescaler+1)*/
		TimHandle.Init.Period            = usPeriod;            /* 用于设置定时器周期 Period = (TIM counter clock / TIM output clock) - 1*/
		TimHandle.Init.ClockDivision     = 0;                   /* 用于指示定时器时钟 (CK_INT) 频率与死区发生器以及数字滤波器（ETR、 TIx）
																															 所使用的死区及采样时钟 (tDTS) 之间的分频比*/
		TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;  /* 用于设置计数模式，向上计数模式 */
//		TimHandle.Init.RepetitionCounter = 0;                   /* 用于设置重复计数器，仅 TIM1 和 TIM8 有，其它定时器没有 */
		TimHandle.Init.AutoReloadPreload = 0;                   /* 用于设置定时器的 ARR 自动重装寄存器是更新事件产生时写入有效 */
		if (HAL_TIM_PWM_DeInit(&TimHandle) != HAL_OK)
		{
		//	Error_Handler();		
		}
		
		if (HAL_TIM_PWM_Init(&TimHandle) != HAL_OK) //使能定时器
		{
			//Error_Handler();
		}
		
		/* 配置定时器PWM输出通道 */
		sConfig.OCMode       = TIM_OCMODE_PWM1;         /* 配置输出比较模式 */
		sConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;     /* 设置输出高电平有效 */
		sConfig.OCFastMode   = TIM_OCFAST_DISABLE;      /* 关闭快速输出模式 */
		sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;    /* 配置互补输出高电平有效 */
		sConfig.OCIdleState  = TIM_OCIDLESTATE_SET;     /* 空闲状态时，设置输出比较引脚为高电平 */
		sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;  /* 空闲状态时，设置互补输出比较引脚为低电平 */
		/* 占空比 */
		sConfig.Pulse = pulse;
		
		if (HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_2) != HAL_OK) //TIM_CHANNEL_1
		{
	//		Error_Handler();
		}
		
		/* 启动PWM输出 */
		if (HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2) != HAL_OK) //TIM_CHANNEL_1
		{
		//	Error_Handler();
		}
}




DMA_HandleTypeDef hdma_spi3_tx;
DMA_HandleTypeDef hdma_spi3_rx;

void AD7606_SPI_DMA_Init()
{
	//一定要发送和接收DMA同时配置，否则无法成功
	__HAL_RCC_DMA1_CLK_ENABLE();	//使能DMA1时钟
	//SPI3 Tx
		/* 关联DMA句柄到SPI */
    __HAL_LINKDMA(&hspi3,hdmatx,hdma_spi3_tx);
		/* SPI3_TX Init */
		hdma_spi3_tx.Instance                 = DMA1_Stream2;   			  /* 例化使用的DMA数据流 */
		hdma_spi3_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;   /* 禁止FIFO*/
		hdma_spi3_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;/* 禁止FIFO此位不起作用，用于设置阀值 */
		hdma_spi3_tx.Init.MemBurst            = DMA_MBURST_SINGLE;	   /* 禁止FIFO此位不起作用，用于存储器突发 */
		hdma_spi3_tx.Init.PeriphBurst         = DMA_PBURST_SINGLE;	   /* 禁止FIFO此位不起作用，用于外设突发 */
		hdma_spi3_tx.Init.Request             = DMA_REQUEST_SPI3_TX;    /* 请求类型 */  
		hdma_spi3_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;   /* 传输方向从存储器到外设 */  
		hdma_spi3_tx.Init.PeriphInc           = DMA_PINC_DISABLE;       /* 外设地址自增禁止 */   
		hdma_spi3_tx.Init.MemInc              = DMA_MINC_ENABLE;        /* 存储器地址自增使能 */   
		hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;    /* 外设数据传输位宽选择字节，即8bit */ 
		hdma_spi3_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;    /* 存储器数据传输位宽选择字节，即8bit */   
		hdma_spi3_tx.Init.Mode                = DMA_NORMAL;             /* 正常模式 */
		hdma_spi3_tx.Init.Priority            = DMA_PRIORITY_VERY_HIGH;      /* 优先级低 */

		HAL_DMA_DeInit(&hdma_spi3_tx);
    if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK)
    {
  //    Error_Handler();
    }

	__HAL_LINKDMA(&hspi3,hdmarx,hdma_spi3_rx);
	/* SPI3_RX Init */
	hdma_spi3_rx.Instance                 = DMA1_Stream1;     /* 例化使用的DMA数据流 */
	hdma_spi3_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;   /* 禁止FIFO*/
	hdma_spi3_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;/* 禁止FIFO此位不起作用，用于设置阀值 */
	hdma_spi3_rx.Init.MemBurst            = DMA_MBURST_SINGLE;	   /* 禁止FIFO此位不起作用，用于存储器突发 */
	hdma_spi3_rx.Init.PeriphBurst         = DMA_PBURST_SINGLE;	   /* 禁止FIFO此位不起作用，用于外设突发 */
	hdma_spi3_rx.Init.Request             = DMA_REQUEST_SPI3_RX;    /* 请求类型 */  
	hdma_spi3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;   /* 传输方向从外设到存储器 */  
	hdma_spi3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;       /* 外设地址自增禁止 */   
	hdma_spi3_rx.Init.MemInc              = DMA_MINC_ENABLE;        /* 存储器地址自增使能 */   
	hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;    /* 外设数据传输位宽选择字节，即8bit */ 
	hdma_spi3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;    /* 存储器数据传输位宽选择字节，即8bit */   
	hdma_spi3_rx.Init.Mode                = DMA_NORMAL;             /* 正常模式 */
	hdma_spi3_rx.Init.Priority            = DMA_PRIORITY_HIGH;      /* 优先级低 */	
	
	HAL_DMA_DeInit(&hdma_spi3_rx);
	if (HAL_DMA_Init(&hdma_spi3_rx) != HAL_OK)
	{
		//Error_Handler();
	}
	
}

/*
*********************************************************************************************************
*	函 数 名: AD7606_Fs_Set
*	功能说明: AD7606采样率设置
*	形    参: Fs：采样率
*	返 回 值: 无
*********************************************************************************************************
*/
float AD7606_Fs_Set(float Fs)
{
	uint16_t usPeriod;
	uint16_t usPrescaler;
	uint32_t pulse;
	
	if (Fs < 100)
	{
		usPrescaler = 10000 - 1;														/* 分频比 = 10000 */
		usPeriod =  (SystemCoreClock/2 / 10000) / Fs  - 1;	/* 自动重装的值， usPeriod最小值168, 单位59us */
		pulse = usPeriod;                               		/* 设置低电平时间59us，注意usPeriod已经进行了减1操作 */
	} 
	else if (Fs < 3000)
	{
		usPrescaler = 100 - 1;															/* 分频比 = 100 */
		usPeriod =  (SystemCoreClock/2 / 100) / Fs  - 1;		/* 自动重装的值， usPeriod最小值560，单位595ns */
		pulse = usPeriod-1;                           			/* 设置低电平时间1.19us，注意usPeriod已经进行了减1操作 */
	}
	else	/* 大于4K的频率，无需分频 */
	{
		usPrescaler = 0;																		/* 分频比 = 1 */
		usPeriod = SystemCoreClock/2 / Fs - 1;							/* 自动重装的值， usPeriod最小值840，单位5.95ns */
		pulse = usPeriod - 199;              								/* 设置低电平时间1.19us，注意usPeriod已经进行了减1操作 */
	}
	
	AD7606_Fs=SystemCoreClock/2.0/(usPrescaler+1)/(usPeriod + 1);//计算实际采样率
	
	TIM3->ARR=usPeriod;
	TIM3->PSC=usPrescaler;
	TIM3->CCR2 = pulse;

	return AD7606_Fs;
	
}


void AD7606_DMA_Read()
{

	  while (BUSY_IS_LOW()==0);
			HAL_SPI_Transmit(&hspi3, (uint8_t*)Spi_Data, AD7606_Channel_Num, 1000);
			if( HAL_SPI_TransmitReceive_DMA(&hspi3, (uint8_t*)Spi_Data, (uint8_t*)&AD7606_Buffer[0][0], AD7606_Channel_Num)== HAL_OK)
			{
			//	printf("DMA开始传输\r\n");
			}
			while(1)
			{
				if(__HAL_DMA_GET_FLAG(&hspi3,DMA_FLAG_TCIF2_6) && __HAL_DMA_GET_FLAG(&hspi3,DMA_FLAG_TCIF1_5))//等待DMA2_Steam7传输完成
				{
						SCB_CleanInvalidateDCache();
					//	HAL_SPI_DMAStop(&hspi3);      //传输完成以后关闭串口DMA
						HAL_SPI_DeInit(&hspi3);
						HAL_SPI_Init(&hspi3);
						HAL_DMA_DeInit(&hdma_spi3_tx);
						HAL_DMA_Init(&hdma_spi3_tx);
						HAL_DMA_DeInit(&hdma_spi3_rx);
						HAL_DMA_Init(&hdma_spi3_rx);
						printf("传输完成\r\n");
			  		DispOnce();
				//	HAL_SPI_DMAStop(&hspi3); 
            DispChall();
						break; 
				}

			}
			

		
}