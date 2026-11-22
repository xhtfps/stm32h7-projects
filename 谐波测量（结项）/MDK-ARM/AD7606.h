#ifndef _Drive_AD7606_DMA_H_
#define _Drive_AD7606_DMA_H_

#include "stm32h7xx_hal.h"

void AD7606_Scan(void);
void AD7606_SPI_Init(void);
void AD7606_SetTIMOutPWM2(uint32_t AD7606_Fs);
void AD7606_SPI_DMA_Init();
void AD7606_DMA_Read();
void DispOnce();
void DispChall();
float AD7606_Fs_Set(float Fs);

#define AD7606_BUFSIZE        32
extern  uint32_t AD7606_Samp_Num;
extern TIM_HandleTypeDef TimHandle ;
//#define AD7606_Samp_Num (256)
#define AD7606_Channel_Num 8

extern int16_t AD7606_Buffer[65536][AD7606_Channel_Num];
extern uint16_t Spi_Data[AD7606_Channel_Num];

extern float AD7606_Fs;
extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_spi3_rx;



#endif
