/*
 * BSP_4x4KEY.h
 *
 *  Created on: 2022年5月28日
 *      Author: Chongmao Wang
 */

#ifndef BSP_INC_BSP_4X4KEY_H_
#define BSP_INC_BSP_4X4KEY_H_

#include "stm32h7xx_hal.h"

// 枚举 按键名称
enum KEY_NAME
{
    KEY_NULL = 0,   // 无按键

    KEY_NUM1,
    KEY_NUM2,
    KEY_NUM3,
    KEY_A,

    KEY_NUM4,
    KEY_NUM5,
    KEY_NUM6,
    KEY_B,

    KEY_NUM7,
    KEY_NUM8,
    KEY_NUM9,
    KEY_C,

    KEY_STAR,   // 星号键
    KEY_NUM0,
    KEY_HASH,   // 井号键
    KEY_D,
};

// ************************************ 宏定义 4x4KEY分配
// 宏定义 端口分配
#define ROW1_Port_Base GPIOC  // 行
#define ROW2_Port_Base GPIOC
#define ROW3_Port_Base GPIOC
#define ROW4_Port_Base GPIOC

#define COL1_Port_Base GPIOE  // 列
#define COL2_Port_Base GPIOE
#define COL3_Port_Base GPIOC
#define COL4_Port_Base GPIOC
// 宏定义 引脚分配
#define ROW1_Pin GPIO_PIN_7     // 行				//D
#define ROW2_Pin GPIO_PIN_5
#define ROW3_Pin GPIO_PIN_3
#define ROW4_Pin GPIO_PIN_1

#define COL1_Pin GPIO_PIN_3     // 列
#define COL2_Pin GPIO_PIN_2
#define COL3_Pin GPIO_PIN_11
#define COL4_Pin GPIO_PIN_9									//*

// ************************************ 宏定义 操作
#define ROW1_SET HAL_GPIO_WritePin(ROW1_Port_Base, ROW1_Pin,GPIO_PIN_SET)
#define ROW1_RESET HAL_GPIO_WritePin(ROW1_Port_Base, ROW1_Pin,GPIO_PIN_RESET)

#define ROW2_SET HAL_GPIO_WritePin(ROW2_Port_Base, ROW2_Pin,GPIO_PIN_SET)
#define ROW2_RESET HAL_GPIO_WritePin(ROW2_Port_Base, ROW2_Pin,GPIO_PIN_RESET)

#define ROW3_SET HAL_GPIO_WritePin(ROW3_Port_Base, ROW3_Pin,GPIO_PIN_SET)
#define ROW3_RESET HAL_GPIO_WritePin(ROW3_Port_Base, ROW3_Pin,GPIO_PIN_RESET)

#define ROW4_SET HAL_GPIO_WritePin(ROW4_Port_Base, ROW4_Pin,GPIO_PIN_SET)
#define ROW4_RESET HAL_GPIO_WritePin(ROW4_Port_Base, ROW4_Pin,GPIO_PIN_RESET)

#define COL1_READ HAL_GPIO_ReadPin( COL1_Port_Base , COL1_Pin )

#define COL2_READ HAL_GPIO_ReadPin( COL2_Port_Base , COL2_Pin )

#define COL3_READ HAL_GPIO_ReadPin( COL3_Port_Base , COL3_Pin )

#define COL4_READ HAL_GPIO_ReadPin( COL4_Port_Base , COL4_Pin )

// ************************************ 函数声明
void Init_KeyBoard( void );

uint8_t Read4X4KEY( void );





#endif /* 1_BSP_INC_BSP_4X4KEY_H_ */






















