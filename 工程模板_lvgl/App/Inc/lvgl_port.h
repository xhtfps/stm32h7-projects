#ifndef H7_TEMPLATE_LVGL_PORT_H
#define H7_TEMPLATE_LVGL_PORT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void LVGL_Port_Init(void);
void LVGL_Port_Lock(void);
void LVGL_Port_Unlock(void);
uint32_t LVGL_Port_Handler(void);

/* Called from the FreeRTOS tick hook at the configured 1 ms tick rate. */
void LVGL_Port_TickHook(void);

typedef struct
{
    uint32_t sequence;
    uint8_t raw_row;
    uint8_t raw_col;
    uint8_t logical_row;
    uint8_t logical_col;
    char symbol;
    uint32_t lv_key;
} LVGL_KeySnapshot;

/* Returns the latest debounced matrix-key event consumed by LVGL. */
void LVGL_Port_GetKeySnapshot(LVGL_KeySnapshot *snapshot);

/* LTDC diagnostics. A non-zero value proves that the display fetch path
   itself has failed; it is not an LVGL dirty-area problem. */
extern volatile uint32_t g_ltdc_fifo_underrun_count;
extern volatile uint32_t g_ltdc_transfer_error_count;

#ifdef __cplusplus
}
#endif

#endif /* H7_TEMPLATE_LVGL_PORT_H */
