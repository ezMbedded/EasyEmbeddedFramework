#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <assert.h>

#define configASSERT(x)   assert(x)

#define configUSE_PREEMPTION                    1
#define configUSE_TICK_HOOK                     0
#define configUSE_IDLE_HOOK                     0
#define configUSE_16_BIT_TICKS                  0
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                (256)          /* words */
#define configTOTAL_HEAP_SIZE                   (128 * 1024)
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0

/* RP2040 / SMP (optional, drop for single-core) */
#define configNUMBER_OF_CORES                   2    /* older kernels: configNUM_CORES */
#define configUSE_CORE_AFFINITY                 1
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_PASSIVE_IDLE_HOOK             0
#define configTICK_CORE                         0

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTimerPendFunctionCall          1

#endif