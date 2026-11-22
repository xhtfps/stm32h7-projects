#ifndef H7_TEMPLATE_MATRIX_KEYPAD_H
#define H7_TEMPLATE_MATRIX_KEYPAD_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATRIX_KEYPAD_ROWS 4U
#define MATRIX_KEYPAD_COLS 4U

/*
 * Connector order supplied for this board:
 *   1: PB13, 2: PB15, 3: PD13, 4: PE2,
 *   5: PB12, 6: PB14, 7: PB2,  8: PD11.
 * The default assumes pins 1..4 are rows and pins 5..8 are columns.
 */
#define KEYPAD_FIRST_GROUP_IS_ROWS 1U

/* The keypad is mounted opposite to the logical layout: rotate it 180 deg. */
#define KEYPAD_ROTATE_180          1U
#define KEYPAD_ROW_REVERSE         1U
#define KEYPAD_COL_REVERSE         1U

typedef struct
{
    uint8_t raw_row;
    uint8_t raw_col;
    uint8_t logical_row;
    uint8_t logical_col;
    char symbol;
} MatrixKeyEvent;

void MatrixKeypad_Init(void);

/* Returns one event only when a new key press is debounced. */
uint8_t MatrixKeypad_ReadEvent(MatrixKeyEvent *event);

/* Returns the currently detected raw matrix position without debounce. */
uint8_t MatrixKeypad_ScanRaw(uint8_t *row, uint8_t *col);

/* Converts a raw row/column to the logical layout after orientation settings. */
char MatrixKeypad_Symbol(uint8_t raw_row, uint8_t raw_col);

#ifdef __cplusplus
}
#endif

#endif /* H7_TEMPLATE_MATRIX_KEYPAD_H */
