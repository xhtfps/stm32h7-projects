/*
 * BSP_4x4KEY.c
 *
 * 用于驱动4x4矩阵键盘
 *
 *  Created on: 2022年5月28日
 *      Author: Chongmao Wang
 */

#include "BSP_4x4KEY.h"

// 初始化4x4按键
void Init_KeyBoard( void )
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能时钟，等待就绪
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    GPIO_InitStruct.Pin 		= ROW1_Pin;					// 引脚  // 行
	GPIO_InitStruct.Mode 	= GPIO_MODE_OUTPUT_PP;	// 推挽输出模式
	GPIO_InitStruct.Pull 	= GPIO_NOPULL;				// 不上下拉
	GPIO_InitStruct.Speed 	= GPIO_SPEED_FREQ_LOW;	// 低速模式
	HAL_GPIO_Init(ROW1_Port_Base, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin 		= ROW2_Pin;
    HAL_GPIO_Init(ROW2_Port_Base, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin 		= ROW3_Pin;
    HAL_GPIO_Init(ROW3_Port_Base, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin 		= ROW4_Pin;
    HAL_GPIO_Init(ROW4_Port_Base, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin 		= COL1_Pin;					// 引脚  // 列
	GPIO_InitStruct.Mode 	= GPIO_MODE_INPUT;	// 推挽输出模式
	GPIO_InitStruct.Pull 	= GPIO_PULLUP;				// 不上下拉
	GPIO_InitStruct.Speed 	= GPIO_SPEED_FREQ_LOW;	// 低速模式
	HAL_GPIO_Init(COL1_Port_Base, &GPIO_InitStruct);
    
        GPIO_InitStruct.Pin 		= COL2_Pin;
    HAL_GPIO_Init(COL2_Port_Base, &GPIO_InitStruct);
    
        GPIO_InitStruct.Pin 		= COL3_Pin;
    HAL_GPIO_Init(COL3_Port_Base, &GPIO_InitStruct);
    
        GPIO_InitStruct.Pin 		= COL4_Pin;
    HAL_GPIO_Init(COL4_Port_Base, &GPIO_InitStruct);
}


uint8_t Read4X4KEY( void )
{
    uint8_t key_val = KEY_NULL;

    // 拉低行,列为低则选中,以此类推
    ROW1_RESET;
    if( !COL1_READ )
        key_val = KEY_NUM1;
    else if( !COL2_READ )
        key_val = KEY_NUM2;
    else if( !COL3_READ )
        key_val = KEY_NUM3;
    else if( !COL4_READ )
        key_val = KEY_A;
    ROW1_SET;

    ROW2_RESET;
    if( !COL1_READ )
        key_val = KEY_NUM4;
    else if( !COL2_READ )
        key_val = KEY_NUM5;
    else if( !COL3_READ )
        key_val = KEY_NUM6;
    else if( !COL4_READ )
        key_val = KEY_B;
    ROW2_SET;

    ROW3_RESET;
    if( !COL1_READ )
        key_val = KEY_NUM7;
    else if( !COL2_READ )
        key_val = KEY_NUM8;
    else if( !COL3_READ )
        key_val = KEY_NUM9;
    else if( !COL4_READ )
        key_val = KEY_C;
    ROW3_SET;

    ROW4_RESET;
    if( !COL1_READ )
        key_val = KEY_STAR;
    else if( !COL2_READ )
        key_val = KEY_NUM0;
    else if( !COL3_READ )
        key_val = KEY_HASH;
    else if( !COL4_READ )
        key_val = KEY_D;
    ROW4_SET;

    return key_val;
}

















