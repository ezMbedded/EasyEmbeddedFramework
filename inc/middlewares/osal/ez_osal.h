/*****************************************************************************
* Filename:         ez_osal.h
* Author:           Hai Nguyen
* Original Date:    15.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_osal.h
 *  @author Hai Nguyen
 *  @date   15.03.2025
 *  @brief  Common interface for OS abstraction layer
 *
 *  @details None
 */

#ifndef _EZ_OSAL_H
#define _EZ_OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#include <stdint.h>
#include "ez_utilities_common.h"

#if (EZ_OSAL == 1)

#if (EZ_FREERTOS_PORT == 1)
#include "FreeRTOS.h"
#elif (EZ_THREADX_PORT == 1)
#include "tx_api.h"
#endif

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define EZ_OSAL_HIGHEST_PRIORITY     0U   /**< Highest priority */
#define EZ_OSAL_LOWEST_PRIORITY      31U  /**< Lowest priority */

#ifndef EZ_OSAL_USE_STATIC
    #if (EZ_FREERTOS_PORT == 1)
    #define EZ_OSAL_USE_STATIC      configSUPPORT_STATIC_ALLOCATION
    #endif
#else
    #if (EZ_FREERTOS_PORT == 1)
        #if ((EZ_OSAL_USE_STATIC == 1) && (configSUPPORT_STATIC_ALLOCATION == 0))
        #error "OSAL configuration and FreeRTOS configuration are not compatible"
        #endif /* (EZ_OSAL_USE_STATIC) & (configSUPPORT_STATIC_ALLOCATION) */
    #endif 
#endif /* EZ_OSAL_USE_STATIC */


#define EZ_OSAL_DEFINE_TASK_HANDLE(NAME, STACK_SIZE, PRIO, FUNC, ARG, RESOURCE) \
    ezOsal_TaskHandle_t NAME = { \
        .task_name = #NAME, \
        .priority = PRIO, \
        .stack_size = STACK_SIZE, \
        .task_function = FUNC, \
        .argument = ARG, \
        .task_handle = NULL, \
        .static_resource = RESOURCE \
    }

#define EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(NAME, MAX_COUNT, RESOURCE) \
    ezOsal_SemaphoreHandle_t NAME = { \
        .max_count = MAX_COUNT, \
        .static_resource = RESOURCE \
    }

#define EZ_OSAL_DEFINE_TIMER_HANDLE(NAME, PERIOD, FUNC, ARG, RESOURCE) \
    ezOsal_TimerHandle_t NAME = { \
        .timer_name = #NAME, \
        .period_ticks = PERIOD, \
        .timer_callback = FUNC, \
        .argument = ARG, \
        .static_resource = RESOURCE \
    }

#define EZ_OSAL_DEFINE_EVENT_HANDLE(NAME, RESOURCE) \
    ezOsal_EventHandle_t NAME = { \
        .static_resource = RESOURCE \
    }

/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/**@brief: Task function pointer
 */
typedef void (*ezOsal_fpTaskFunction)(void *argument);


/**@brief: Timer elapsed callback function pointer
 */
typedef void (*ezOsal_fpTimerElapseCallback)(void *argument);


/**@brief OSAL task configuration structure
 */
typedef struct
{
    const char* task_name;  /**< Task name */
    uint32_t priority;      /**< Priority */
    uint32_t stack_size;    /**< Stack size */
    ezOsal_fpTaskFunction task_function; /**< Task function */
    void *argument;         /**< Extra argument */
    void *task_handle;      /**< RTOS Task handle */
    void *static_resource;  /**< Point to static resource if RTOS use this mode */
}ezOsal_TaskHandle_t;

/**@brief: OSAL semaphore configuration structure
 */
typedef struct
{
    uint32_t max_count;
    void *static_resource;
    void *handle;
}ezOsal_SemaphoreHandle_t;

typedef struct
{
    const char* timer_name;
    uint32_t period_ticks;
    ezOsal_fpTimerElapseCallback timer_callback;
    void *handle;
    void *argument;
    void* static_resource;
}ezOsal_TimerHandle_t;


typedef struct
{
    void *handle;
    void *static_resource;
}ezOsal_EventHandle_t;

/* Interfaces ***************************************************************/

/**@brief: OSAL interface initialization function
 * @param: argument - Extra argument
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpInit)     (void *argument);

/**@brief: Task create function
 * @param: handle - Task handle
 * @return: Task Status
 */
typedef ezSTATUS (*ezOsal_fpTaskCreate)  (ezOsal_TaskHandle_t* handle);

/**@brief: Task delete function
 * @param: task_handle - Task handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpTaskDelete)     (ezOsal_TaskHandle_t* handle);

/**@brief: Task suspend function
 * @param: task_handle - Task handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpTaskSuspend)    (ezOsal_TaskHandle_t* handle);

/**@brief: Task resume function
 * @param: task_handle - Task handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpTaskResume)     (ezOsal_TaskHandle_t* handle);

/**@brief: Task delay function
 * @param: num_of_ticks - Number of ticks
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpTaskDelay)      (unsigned long num_of_ticks);

/**@brief: Task start scheduler function
 */
typedef void (*ezOsal_fpTaskStartScheduler)(void);

/**@brief: Task get tick count function
 * @return: Tick count
 */
typedef unsigned long (*ezOsal_fpTaskGetTickCount)(void);

/**@brief: Semaphore create function
 * @param: handle: Semaphore handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpSemaphoreCreate)(ezOsal_SemaphoreHandle_t *handle);

/**@brief: Semaphore delete function
 * @param: semaphore_handle - Semaphore handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpSemaphoreDelete)(ezOsal_SemaphoreHandle_t *handle);

/**@brief: Semaphore take function
 * @param: semaphore_handle - Semaphore handle
 * @param: timeout_ticks - Timeout in ticks
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpSemaphoreTake)(ezOsal_SemaphoreHandle_t *handle, uint32_t timeout_ticks);

/**@brief: Semaphore give function
 * @param: semaphore_handle - Semaphore handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpSemaphoreGive)(ezOsal_SemaphoreHandle_t *handle);

/**@brief: Timer create function
 * @param: handle - Timer handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_pfTimerCreate)(ezOsal_TimerHandle_t *handle);

/**@brief: Timer delete function
 * @param: timer_handle - Timer handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_pfTimerDelete)(ezOsal_TimerHandle_t *handle);

/**@brief: Timer start function
 * @param: timer_handle - Timer handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_pfTimerStart)(ezOsal_TimerHandle_t *handle);

/**@brief: Timer stop function
 * @param: timer_handle - Timer handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_pfTimerStop)(ezOsal_TimerHandle_t *handle);

/**@brief: Create new event group
 * @param: handle - Event handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpEventCreate)(ezOsal_EventHandle_t *handle);

/**@brief: Delete event group
 * @param: handle - Event handle
 * @return: Status
 */
typedef ezSTATUS (*ezOsal_fpEventDelete)(ezOsal_EventHandle_t *handle);

/**@brief: Wait for events
 * @param: handle Event handle
 * @return Status
 */
typedef int (*ezOsal_fpEventWait)(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks);

/**@brief: Set event
 * @param: handle Event handle
 * @return Status
 */
typedef ezSTATUS (*ezOsal_fpEventSet)(ezOsal_EventHandle_t *handle, uint32_t event_mask);

/**@brief: Clear events
 * @param: handle Event handle
 * @return Status
 */
typedef ezSTATUS (*ezOsal_fpEventClear)(ezOsal_EventHandle_t *handle, uint32_t event_mask);


/** @brief List of interface functions must be implemented
 */
typedef struct
{
    ezOsal_fpInit           Init;
    /* Task functions */
    ezOsal_fpTaskCreate     TaskCreate;
    ezOsal_fpTaskDelete     TaskDelete;
    ezOsal_fpTaskResume     TaskResume;
    ezOsal_fpTaskSuspend    TaskSuspend;
    ezOsal_fpTaskDelay      TaskDelay;
    ezOsal_fpTaskGetTickCount TaskGetTickCount;
    ezOsal_fpTaskStartScheduler TaskStartScheduler;

    /* Semaphore functions */
    ezOsal_fpSemaphoreCreate SemaphoreCreate;
    ezOsal_fpSemaphoreDelete SemaphoreDelete;
    ezOsal_fpSemaphoreTake   SemaphoreTake;
    ezOsal_fpSemaphoreGive   SemaphoreGive;

    /* Timer functions */
    ezOsal_pfTimerCreate    TimerCreate;
    ezOsal_pfTimerDelete    TimerDelete;
    ezOsal_pfTimerStart     TimerStart;
    ezOsal_pfTimerStop      TimerStop;

    /* Event functions */
    ezOsal_fpEventCreate    EventCreate;
    ezOsal_fpEventDelete    EventDelete;
    ezOsal_fpEventWait      EventWait;
    ezOsal_fpEventSet       EventSet;
    ezOsal_fpEventClear     EventClear;

    /* Extra interface */
    void *custom_interfaces;
}ezOsal_Interfaces_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/
ezSTATUS ezOsal_SetInterface(const ezOsal_Interfaces_t *interface);
ezSTATUS ezOsal_Init(void *argument);

ezSTATUS ezOsal_TaskCreate(ezOsal_TaskHandle_t* handle);
ezSTATUS ezOsal_TaskDelete(ezOsal_TaskHandle_t* handle);
ezSTATUS ezOsal_TaskSuspend(ezOsal_TaskHandle_t* handle);
ezSTATUS ezOsal_TaskResume(ezOsal_TaskHandle_t* handle);
ezSTATUS ezOsal_TaskDelay(unsigned long num_of_ticks);
unsigned long ezOsal_TaskGetTickCount(void);
void ezOsal_TaskStartScheduler(void);

ezSTATUS ezOsal_SemaphoreCreate(ezOsal_SemaphoreHandle_t *handle);
ezSTATUS ezOsal_SemaphoreDelete(ezOsal_SemaphoreHandle_t *handle);
ezSTATUS ezOsal_SemaphoreTake(ezOsal_SemaphoreHandle_t *handle, uint32_t timeout_ticks);
ezSTATUS ezOsal_SemaphoreGive(ezOsal_SemaphoreHandle_t *handle);

ezSTATUS ezOsal_TimerCreate(ezOsal_TimerHandle_t *handle);
ezSTATUS ezOsal_TimerDelete(ezOsal_TimerHandle_t *handle);
ezSTATUS ezOsal_TimerStart(ezOsal_TimerHandle_t *handle);
ezSTATUS ezOsal_TimerStop(ezOsal_TimerHandle_t *handle);


ezSTATUS ezOsal_EventCreate(ezOsal_EventHandle_t *handle);
ezSTATUS ezOsal_EventDelete(ezOsal_EventHandle_t *handle);
int ezOsal_EventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks);
ezSTATUS ezOsal_EventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask);
ezSTATUS ezOsal_EventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask);

#endif /* EZ_OSAL == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_OSAL_H */

/* End of file */
