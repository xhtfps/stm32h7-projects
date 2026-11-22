#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32h7xx.h"
#include <stdint.h>

extern uint32_t SystemCoreClock;

/* Kernel scheduling. */
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                     SystemCoreClock
#define configTICK_RATE_HZ                     1000U
#define configMAX_PRIORITIES                   8U
#define configMINIMAL_STACK_SIZE               256U
#define configMAX_TASK_NAME_LEN                16U
#define configUSE_16_BIT_TICKS                 0
#define configIDLE_SHOULD_YIELD                1

/* Kernel objects and API features used by the application. */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configQUEUE_REGISTRY_SIZE               8U
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1U
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0U

/* Dynamic allocation uses the FreeRTOS heap_4 implementation. */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                   ((size_t)(64U * 1024U))
#define configAPPLICATION_ALLOCATED_HEAP         0

/* Diagnostics. */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0

/* Software timers are not required by the initial port. */
#define configUSE_TIMERS                        0

/* Optional API functions. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/* Cortex-M interrupt priority mapping: priorities 0..4 must not call RTOS APIs. */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                     4U
#endif
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5U
/* __NVIC_PRIO_BITS is 4 on STM32H743: 15 << 4 = 240, 5 << 4 = 80. */
#define configKERNEL_INTERRUPT_PRIORITY      0xF0
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 0x50

/* FreeRTOS supplies the real handlers from the Cortex-M7 port. */
#define xPortPendSVHandler  PendSV_Handler
#define vPortSVCHandler     SVC_Handler

#define configASSERT(x) \
    do { \
        if ((x) == 0) { \
            __disable_irq(); \
            for (;;) { } \
        } \
    } while (0)

#endif /* FREERTOS_CONFIG_H */
