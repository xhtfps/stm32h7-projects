#include "freertos_app.h"

#include "FreeRTOS.h"
#include "app.h"
#include "task.h"

#define APP_TASK_PRIORITY       (tskIDLE_PRIORITY + 1U)
#define APP_TASK_STACK_WORDS    1024U

static void app_task(void *argument)
{
    (void)argument;

    App_Init();

    for (;;)
    {
        App_Loop();
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
}

static void freertos_fatal(void)
{
    __disable_irq();
    for (;;) { }
}

void FreeRTOS_AppStart(void)
{
    if (xTaskCreate(app_task,
                    "app",
                    APP_TASK_STACK_WORDS,
                    NULL,
                    APP_TASK_PRIORITY,
                    NULL) != pdPASS)
    {
        freertos_fatal();
    }

    vTaskStartScheduler();

    /* The scheduler only returns when the idle task could not be created. */
    freertos_fatal();
}

void vApplicationMallocFailedHook(void)
{
    freertos_fatal();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    freertos_fatal();
}
