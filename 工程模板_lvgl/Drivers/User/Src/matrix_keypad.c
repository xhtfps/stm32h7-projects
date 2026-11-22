#include "matrix_keypad.h"

#define MATRIX_KEYPAD_DEBOUNCE_MS 25U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} MatrixGpio;

static const MatrixGpio g_pin_group_a[MATRIX_KEYPAD_ROWS] =
{
    {GPIOB, GPIO_PIN_13},
    {GPIOB, GPIO_PIN_15},
    {GPIOD, GPIO_PIN_13},
    {GPIOE, GPIO_PIN_2}
};

static const MatrixGpio g_pin_group_b[MATRIX_KEYPAD_COLS] =
{
    {GPIOB, GPIO_PIN_12},
    {GPIOB, GPIO_PIN_14},
    {GPIOB, GPIO_PIN_2},
    {GPIOD, GPIO_PIN_11}
};

static uint8_t g_initialized;
static uint8_t g_last_pressed;
static uint8_t g_stable_pressed;
static uint8_t g_last_row;
static uint8_t g_last_col;
static uint32_t g_changed_tick;

static uint8_t apply_orientation(uint8_t raw_row,
                                  uint8_t raw_col,
                                  uint8_t *logical_row,
                                  uint8_t *logical_col)
{
    uint8_t row = raw_row;
    uint8_t col = raw_col;

    if ((logical_row == 0) || (logical_col == 0) ||
        (raw_row >= MATRIX_KEYPAD_ROWS) || (raw_col >= MATRIX_KEYPAD_COLS))
    {
        return 0U;
    }

#if KEYPAD_ROTATE_180
    row = (MATRIX_KEYPAD_ROWS - 1U) - row;
    col = (MATRIX_KEYPAD_COLS - 1U) - col;
#endif
#if KEYPAD_ROW_REVERSE
    row = (MATRIX_KEYPAD_ROWS - 1U) - row;
#endif
#if KEYPAD_COL_REVERSE
    col = (MATRIX_KEYPAD_COLS - 1U) - col;
#endif

    *logical_row = row;
    *logical_col = col;
    return 1U;
}

static const MatrixGpio *row_gpio(uint8_t index)
{
#if KEYPAD_FIRST_GROUP_IS_ROWS
    return &g_pin_group_a[index];
#else
    return &g_pin_group_b[index];
#endif
}

static const MatrixGpio *col_gpio(uint8_t index)
{
#if KEYPAD_FIRST_GROUP_IS_ROWS
    return &g_pin_group_b[index];
#else
    return &g_pin_group_a[index];
#endif
}

static void enable_gpio_clocks(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
}

static void set_all_rows(GPIO_PinState state)
{
    uint8_t index;

    for (index = 0U; index < MATRIX_KEYPAD_ROWS; ++index)
    {
        HAL_GPIO_WritePin(row_gpio(index)->port, row_gpio(index)->pin, state);
    }
}

static void settle_gpio(void)
{
    volatile uint32_t count = 20U;

    while (count-- != 0U)
    {
        __NOP();
    }
}

void MatrixKeypad_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t index;

    if (g_initialized != 0U)
    {
        return;
    }

    enable_gpio_clocks();

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    for (index = 0U; index < MATRIX_KEYPAD_ROWS; ++index)
    {
        gpio.Pin = row_gpio(index)->pin;
        HAL_GPIO_Init(row_gpio(index)->port, &gpio);
    }

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    for (index = 0U; index < MATRIX_KEYPAD_COLS; ++index)
    {
        gpio.Pin = col_gpio(index)->pin;
        HAL_GPIO_Init(col_gpio(index)->port, &gpio);
    }

    set_all_rows(GPIO_PIN_SET);
    g_last_pressed = 0U;
    g_stable_pressed = 0U;
    g_changed_tick = HAL_GetTick();
    g_initialized = 1U;
}

uint8_t MatrixKeypad_ScanRaw(uint8_t *row, uint8_t *col)
{
    uint8_t row_index;
    uint8_t col_index;

    if ((g_initialized == 0U) || (row == 0) || (col == 0))
    {
        return 0U;
    }

    set_all_rows(GPIO_PIN_SET);
    for (row_index = 0U; row_index < MATRIX_KEYPAD_ROWS; ++row_index)
    {
        set_all_rows(GPIO_PIN_SET);
        HAL_GPIO_WritePin(row_gpio(row_index)->port,
                          row_gpio(row_index)->pin,
                          GPIO_PIN_RESET);
        settle_gpio();

        for (col_index = 0U; col_index < MATRIX_KEYPAD_COLS; ++col_index)
        {
            if (HAL_GPIO_ReadPin(col_gpio(col_index)->port,
                                 col_gpio(col_index)->pin) == GPIO_PIN_RESET)
            {
                *row = row_index;
                *col = col_index;
                set_all_rows(GPIO_PIN_SET);
                return 1U;
            }
        }
    }

    set_all_rows(GPIO_PIN_SET);
    return 0U;
}

char MatrixKeypad_Symbol(uint8_t raw_row, uint8_t raw_col)
{
    static const char layout[MATRIX_KEYPAD_ROWS][MATRIX_KEYPAD_COLS] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    uint8_t logical_row;
    uint8_t logical_col;

    if (apply_orientation(raw_row, raw_col, &logical_row, &logical_col) == 0U)
    {
        return '?';
    }

    return layout[logical_row][logical_col];
}

uint8_t MatrixKeypad_ReadEvent(MatrixKeyEvent *event)
{
    uint8_t row = 0U;
    uint8_t col = 0U;
    uint8_t pressed;
    uint32_t now;

    if ((event == 0) || (g_initialized == 0U))
    {
        return 0U;
    }

    pressed = MatrixKeypad_ScanRaw(&row, &col);
    now = HAL_GetTick();

    if ((pressed != g_last_pressed) ||
        ((pressed != 0U) && ((row != g_last_row) || (col != g_last_col))))
    {
        g_last_pressed = pressed;
        g_last_row = row;
        g_last_col = col;
        g_changed_tick = now;
        return 0U;
    }

    if ((now - g_changed_tick) < MATRIX_KEYPAD_DEBOUNCE_MS)
    {
        return 0U;
    }

    if ((pressed != 0U) && (g_stable_pressed == 0U))
    {
        g_stable_pressed = 1U;
        event->raw_row = row;
        event->raw_col = col;
        (void)apply_orientation(row,
                                col,
                                &event->logical_row,
                                &event->logical_col);
        event->symbol = MatrixKeypad_Symbol(row, col);
        return 1U;
    }

    if (pressed == 0U)
    {
        g_stable_pressed = 0U;
    }

    return 0U;
}
