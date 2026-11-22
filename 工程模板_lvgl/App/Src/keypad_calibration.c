#include "keypad_calibration.h"
#include "lcd_fonts.h"
#include "lcd_rgb.h"
#include "matrix_keypad.h"
#include <stdio.h>

static void display_status(const char *text, uint32_t color)
{
    LCD_SetColor(color);
    LCD_ClearRect(20U, 330U, 760U, 35U);
    LCD_DisplayString(20U, 330U, (char *)text);
}

void KeypadCalibration_Init(void)
{
#if LCD_NUM_LAYERS == 2
    /* MX_LTDC_Init clears layer1 with opaque black. Make it transparent so
       the calibration UI drawn on layer0 is visible. */
    LCD_SetLayer(1U);
    LCD_SetBackColor(LCD_BLACK & 0x00FFFFFFUL);
    LCD_Clear();
#endif

    LCD_SetLayer(0U);
    LCD_DisplayDirection(Direction_H);
    LCD_SetBackColor(LCD_BLACK);
    LCD_SetColor(LCD_WHITE);
    LCD_SetFont(&Font20);
    LCD_Clear();

    LCD_DisplayString(20U, 20U, "4x4 KEYPAD CALIBRATION");
    LCD_DisplayString(20U, 55U, "ROW: PB13 PB15 PD13 PE2");
    LCD_DisplayString(20U, 85U, "COL: PB12 PB14 PB2 PD11");
#if KEYPAD_FIRST_GROUP_IS_ROWS
    LCD_DisplayString(20U, 115U, "WIRING: 1-4 ROW, 5-8 COL");
#else
    LCD_DisplayString(20U, 115U, "WIRING: 1-4 COL, 5-8 ROW");
#endif
    LCD_DisplayString(20U, 160U, "MAP: 1 2 3 A");
    LCD_DisplayString(20U, 190U, "     4 5 6 B");
    LCD_DisplayString(20U, 220U, "     7 8 9 C");
    LCD_DisplayString(20U, 250U, "     * 0 # D");
    display_status("READY: PRESS ANY KEY", LCD_GREEN);
}

void KeypadCalibration_Loop(void)
{
    MatrixKeyEvent event;
    char line[64];

    if (MatrixKeypad_ReadEvent(&event) != 0U)
    {
        sprintf(line, "KEY %c RAW R%u C%u LOG R%u C%u",
                event.symbol,
                (unsigned)event.raw_row,
                (unsigned)event.raw_col,
                (unsigned)event.logical_row,
                (unsigned)event.logical_col);
        display_status(line, LCD_YELLOW);
    }

}
