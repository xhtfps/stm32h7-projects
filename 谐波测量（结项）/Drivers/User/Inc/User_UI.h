#ifndef USER_INC_USER_UI_H_
#define USER_INC_USER_UI_H_

#include "stm32h7xx_hal.h"
#include "lcd_rgb.h"
// 枚举 界面名称
enum UI_NAME
{
    UI_NULL = 0,

    UI_START,
    UI_LOWPOW,

    UI_TA_MENU,
    UI_TB_MENU,
    UI_TC_MENU,
    UI_TD_MENU,
};

void User_UpdataUI( uint8_t ui_name );

#endif
