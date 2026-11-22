#include "app.h"
#include "lvgl_port.h"
#include "lvgl_ui.h"
#include "lcd_rgb.h"
#include "matrix_keypad.h"
#include "sdram.h"

void App_Init(void)
{
    MX_FMC_Init();
    MX_LTDC_Init();
    MatrixKeypad_Init();
    LVGL_Port_Init();
    LVGL_Port_Lock();
    LVGL_UI_Init();
    LVGL_Port_Unlock();
}

uint32_t App_Loop(void)
{
    return LVGL_Port_Handler();
}
