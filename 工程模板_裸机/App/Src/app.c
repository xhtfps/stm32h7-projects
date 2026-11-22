#include "app.h"
#include "keypad_calibration.h"
#include "lcd_rgb.h"
#include "matrix_keypad.h"
#include "sdram.h"

void App_Init(void)
{
    MX_FMC_Init();
    MX_LTDC_Init();
    MatrixKeypad_Init();
    KeypadCalibration_Init();
}

void App_Loop(void)
{
    KeypadCalibration_Loop();
}
