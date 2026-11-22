#ifndef H7_TEMPLATE_MAIN_H
#define H7_TEMPLATE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

/* Board-independent switches. Set to 0 when the corresponding hardware is absent. */
#define H7_TEMPLATE_USE_EXTERNAL_SDRAM 1U
#define H7_TEMPLATE_USE_LTDC_CLOCK     1U

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* H7_TEMPLATE_MAIN_H */
