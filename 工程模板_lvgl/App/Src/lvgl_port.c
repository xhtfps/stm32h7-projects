#include "lvgl_port.h"

#include "FreeRTOS.h"
#include "lcd_rgb.h"
#include "matrix_keypad.h"
#include "semphr.h"
#include "sdram.h"
#include "string.h"

#define LVGL_DISPLAY_WIDTH       800U
#define LVGL_DISPLAY_HEIGHT      480U
/* Keep both complete RGB565 framebuffers in the two reserved LCD layer
   regions. LVGL renders into the back buffer; LTDC scans the front buffer.
   The address is exchanged only on vertical blanking, so DMA2D/LVGL never
   writes the framebuffer currently being read by LTDC. */
#define LVGL_DRAW_BUFFER_LINES   LVGL_DISPLAY_HEIGHT
#define LVGL_FRAME_PIXELS        (LVGL_DISPLAY_WIDTH * LVGL_DISPLAY_HEIGHT)

static SemaphoreHandle_t g_lvgl_mutex;
static lv_indev_t *g_keypad_indev;
static lv_disp_draw_buf_t g_draw_buffer;
static lv_color_t *g_front_buffer;
static lv_color_t *g_back_buffer;
static uint32_t g_last_key;
static volatile LVGL_KeySnapshot g_key_snapshot;
volatile uint32_t g_ltdc_fifo_underrun_count;
volatile uint32_t g_ltdc_transfer_error_count;
extern DMA2D_HandleTypeDef hdma2d;
extern LTDC_HandleTypeDef hltdc;

static void display_flush(lv_disp_drv_t *disp_drv,
                          const lv_area_t *area,
                          lv_color_t *color_p);
static void keypad_read(lv_indev_drv_t *indev_drv,
                        lv_indev_data_t *data);
static uint32_t keypad_to_lv_key(char symbol);
static void cache_clean_range(void *address, uint32_t length);
static void cache_invalidate_range(void *address, uint32_t length);
static void dma2d_init(void);

void HAL_LTDC_ErrorCallback(LTDC_HandleTypeDef *handle)
{
    uint32_t error_code;

    if (handle == NULL)
    {
        return;
    }

    error_code = handle->ErrorCode;
    if ((error_code & HAL_LTDC_ERROR_FU) != 0U)
    {
        ++g_ltdc_fifo_underrun_count;
    }
    if ((error_code & HAL_LTDC_ERROR_TE) != 0U)
    {
        ++g_ltdc_transfer_error_count;
    }
}

static void dma2d_init(void)
{
    memset(&hdma2d, 0, sizeof(hdma2d));
    hdma2d.Instance = DMA2D;
    hdma2d.Init.Mode = DMA2D_M2M;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = 0U;
    hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
    hdma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;
    hdma2d.Init.BytesSwap = DMA2D_BYTES_REGULAR;
    hdma2d.Init.LineOffsetMode = DMA2D_LOM_PIXELS;
    configASSERT(HAL_DMA2D_Init(&hdma2d) == HAL_OK);

    hdma2d.LayerCfg[1].InputOffset = 0U;
    hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_RGB565;
    hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
    hdma2d.LayerCfg[1].InputAlpha = 0xFFU;
    hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
    hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;
    hdma2d.LayerCfg[1].ChromaSubSampling = DMA2D_NO_CSS;
    configASSERT(HAL_DMA2D_ConfigLayer(&hdma2d, 1U) == HAL_OK);
}

static void cache_clean_range(void *address, uint32_t length)
{
    uintptr_t start;
    uintptr_t aligned_start;
    uintptr_t aligned_end;

    if ((address == NULL) || (length == 0U))
    {
        return;
    }

    start = (uintptr_t)address;
    aligned_start = start & ~(uintptr_t)31U;
    aligned_end = (start + (uintptr_t)length + 31U) & ~(uintptr_t)31U;
    SCB_CleanDCache_by_Addr((uint32_t *)aligned_start,
                            (int32_t)(aligned_end - aligned_start));
}

static void cache_invalidate_range(void *address, uint32_t length)
{
    uintptr_t start;
    uintptr_t aligned_start;
    uintptr_t aligned_end;

    if ((address == NULL) || (length == 0U))
    {
        return;
    }

    start = (uintptr_t)address;
    aligned_start = start & ~(uintptr_t)31U;
    aligned_end = (start + (uintptr_t)length + 31U) & ~(uintptr_t)31U;
    SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_start,
                                  (int32_t)(aligned_end - aligned_start));
}

static void display_flush(lv_disp_drv_t *disp_drv,
                          const lv_area_t *area,
                          lv_color_t *color_p)
{
    uint32_t width;
    uint32_t height;
    uint32_t row;
    uint16_t *framebuffer;
    lv_color_t *old_front_buffer;
    uint16_t *destination;

    (void)disp_drv;

    width = (uint32_t)(area->x2 - area->x1 + 1);
    height = (uint32_t)(area->y2 - area->y1 + 1);
    framebuffer = (uint16_t *)(uintptr_t)g_back_buffer;

    /* Full-screen flush: publish the completed back buffer at the next
       vertical blanking interval. This is the important synchronization
       boundary; copying into the currently displayed buffer can create fixed
       horizontal black bands when LTDC and DMA2D contend for SDRAM. */
    if ((color_p == g_back_buffer) &&
        (area->x1 == 0) &&
        (area->y1 == 0) &&
        (area->x2 == (lv_coord_t)(LVGL_DISPLAY_WIDTH - 1U)) &&
        (area->y2 == (lv_coord_t)(LVGL_DISPLAY_HEIGHT - 1U)))
    {
        cache_clean_range(g_back_buffer,
                          LVGL_FRAME_PIXELS * sizeof(uint16_t));
        __DSB();
        configASSERT(HAL_LTDC_SetAddress_NoReload(&hltdc,
                                                  (uint32_t)(uintptr_t)g_back_buffer,
                                                  0U) == HAL_OK);
        configASSERT(HAL_LTDC_Reload(&hltdc,
                                     LTDC_RELOAD_VERTICAL_BLANKING) == HAL_OK);

        old_front_buffer = g_front_buffer;
        g_front_buffer = g_back_buffer;
        g_back_buffer = old_front_buffer;
        g_draw_buffer.buf1 = g_back_buffer;
        g_draw_buffer.buf_act = g_back_buffer;
        lv_disp_flush_ready(disp_drv);
        return;
    }

    /* LVGL's DMA2D renderer changes CR/FGPFCCR while drawing. Reinitialize
       the HAL state so the flush is a real M2M RGB565 copy, not the R2M mode
       used by the renderer's color-fill operation. */
    hdma2d.Init.OutputOffset = 0U;
    configASSERT(HAL_DMA2D_Init(&hdma2d) == HAL_OK);
    configASSERT(HAL_DMA2D_ConfigLayer(&hdma2d, 1U) == HAL_OK);

    /* Keep the transfer one line at a time. LVGL's partial buffer is tightly
       packed, while the LTDC framebuffer has an 800-pixel stride. */
    for (row = 0U; row < height; ++row)
    {
        destination = &framebuffer[((uint32_t)area->y1 + row) * LVGL_DISPLAY_WIDTH +
                                   (uint32_t)area->x1];
        cache_invalidate_range(destination, width * sizeof(uint16_t));
        cache_clean_range(color_p, width * sizeof(uint16_t));
        configASSERT(HAL_DMA2D_Start(&hdma2d,
                                     (uint32_t)(uintptr_t)color_p,
                                     (uint32_t)(uintptr_t)destination,
                                     width,
                                     1U) == HAL_OK);
        configASSERT(HAL_DMA2D_PollForTransfer(&hdma2d, 100U) == HAL_OK);
        cache_invalidate_range(destination, width * sizeof(uint16_t));
        color_p += width;
    }

    /* LVGL must be told when the flush callback has finished. */
    lv_disp_flush_ready(disp_drv);
}

static uint32_t keypad_to_lv_key(char symbol)
{
    /* Keep numeric symbols intact. The UI uses them as password digits on
       the lock screen and translates them to focus movement on the home
       screen. Mapping them here to LV_KEY_UP/DOWN makes the lock screen
       lose numeric input because LVGL consumes navigation keys first. */
    if ((symbol >= '0') && (symbol <= '9'))
    {
        return (uint32_t)(uint8_t)symbol;
    }

    switch (symbol)
    {
    case 'A': return LV_KEY_ENTER;
    case 'B': return LV_KEY_BACKSPACE;
    case 'C': return LV_KEY_ESC;
    case 'D': return LV_KEY_DEL;
    case '*': return LV_KEY_HOME;
    case '#': return LV_KEY_END;
    default:  return (uint32_t)(uint8_t)symbol;
    }
}

static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    MatrixKeyEvent event;
    uint32_t key;

    (void)indev_drv;
    key = 0U;

    if (MatrixKeypad_ReadEvent(&event) != 0U)
    {
        key = keypad_to_lv_key(event.symbol);
        if (key != 0U)
        {
            g_last_key = key;
            g_key_snapshot.raw_row = event.raw_row;
            g_key_snapshot.raw_col = event.raw_col;
            g_key_snapshot.logical_row = event.logical_row;
            g_key_snapshot.logical_col = event.logical_col;
            g_key_snapshot.symbol = event.symbol;
            g_key_snapshot.lv_key = key;
            g_key_snapshot.sequence++;
            data->state = LV_INDEV_STATE_PR;
        }
        else
        {
            data->state = LV_INDEV_STATE_REL;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    data->key = g_last_key;
}

void LVGL_Port_Init(void)
{
    static lv_disp_drv_t display_driver;
    static lv_indev_drv_t keypad_driver;
    static lv_group_t *keypad_group;

    g_lvgl_mutex = xSemaphoreCreateMutex();
    configASSERT(g_lvgl_mutex != NULL);

    dma2d_init();

    g_front_buffer = (lv_color_t *)(uintptr_t)LCD_MemoryAdd;
    g_back_buffer = (lv_color_t *)(uintptr_t)(LCD_MemoryAdd +
                                               LCD_MemoryAdd_OFFSET);

#if LCD_NUM_LAYERS == 2
    /* LVGL owns layer0. Layer1 is unused; remove its SDRAM read stream so the
       LTDC has the maximum margin against FIFO underrun while DMA2D updates
       the framebuffer. The explicit reload is required after changing LEN. */
    configASSERT(HAL_LTDC_SetAlpha(&hltdc, 0U, 1U) == HAL_OK);
    __HAL_LTDC_LAYER_DISABLE(&hltdc, 1U);
    __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
    __HAL_LTDC_CLEAR_FLAG(&hltdc, LTDC_FLAG_FU | LTDC_FLAG_TE);
    HAL_NVIC_SetPriority(LTDC_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);
#endif

    lv_init();

    lv_disp_draw_buf_init(&g_draw_buffer,
                          g_back_buffer,
                          NULL,
                          LVGL_DISPLAY_WIDTH * LVGL_DRAW_BUFFER_LINES);
    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = LVGL_DISPLAY_WIDTH;
    display_driver.ver_res = LVGL_DISPLAY_HEIGHT;
    display_driver.flush_cb = display_flush;
    display_driver.draw_buf = &g_draw_buffer;
    /* Render a complete frame, then atomically publish it at VBlank. */
    display_driver.full_refresh = 1U;
    lv_disp_drv_register(&display_driver);

    lv_indev_drv_init(&keypad_driver);
    keypad_driver.type = LV_INDEV_TYPE_KEYPAD;
    keypad_driver.read_cb = keypad_read;
    g_keypad_indev = lv_indev_drv_register(&keypad_driver);

    keypad_group = lv_group_create();
    lv_group_set_default(keypad_group);
    lv_indev_set_group(g_keypad_indev, keypad_group);
}

void LVGL_Port_Lock(void)
{
    if (g_lvgl_mutex != NULL)
    {
        (void)xSemaphoreTake(g_lvgl_mutex, portMAX_DELAY);
    }
}

void LVGL_Port_Unlock(void)
{
    if (g_lvgl_mutex != NULL)
    {
        (void)xSemaphoreGive(g_lvgl_mutex);
    }
}

uint32_t LVGL_Port_Handler(void)
{
    uint32_t next_period;

    LVGL_Port_Lock();
    next_period = lv_timer_handler();
    LVGL_Port_Unlock();

    /* Avoid a busy loop while retaining responsive keypad input and smooth
       LVGL animations. The caller clamps this value to the RTOS tick. */
    if (next_period == 0U)
    {
        next_period = 1U;
    }
    return next_period;
}

void LVGL_Port_TickHook(void)
{
    lv_tick_inc(1U);
}

void LVGL_Port_GetKeySnapshot(LVGL_KeySnapshot *snapshot)
{
    if (snapshot != NULL)
    {
        *snapshot = g_key_snapshot;
    }
}
