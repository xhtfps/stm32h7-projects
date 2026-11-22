#include "lcd_fonts.h"

/*
 * The active UI is rendered by LVGL and does not use the legacy LCD bitmap
 * text renderer. Keep the RGB LCD driver's default-font symbols available
 * without pulling the old UTF-8 test font tables into the production target.
 */
pFONT Font12 = {0};
pFONT Font16 = {0};
pFONT Font20 = {0};
pFONT Font24 = {0};
pFONT Font32 = {0};
