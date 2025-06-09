/*****************************************************************************
* Filename:         ez_osal_freertos.c
* Author:           Hai Nguyen
* Original Date:    16.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_osal_freertos.c
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Provide the OSAL interface for FreeRTOS
 *
 *  @details None
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_osal_freertos.h"

#if (EZ_FREERTOS_PORT == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_OSAL_FREERTOS_LOGGING_LEVEL /**< logging level */
#define MOD_NAME    "ez_osal_freertos"  /**< module name */
#include "ez_logging.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "ez_assert.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static ezSTATUS ezOsal_FreeRTOSTaskCreate(ezOsal_TaskHandle_t* handle);
static ezSTATUS ezOsal_FreeRTOSTaskDelete(ezOsal_TaskHandle_t* handle);
static ezSTATUS ezOsal_FreeRTOSTaskSuspend(ezOsal_TaskHandle_t* handle);
static ezSTATUS ezOsal_FreeRTOSTaskResume(ezOsal_TaskHandle_t* handle);
static ezSTATUS ezOsal_FreeRTOSTaskDelay(unsigned long num_of_ticks);
static unsigned long ezOsal_FreeRTOSTaskGetTickCount(void);
static void ezOsal_FreeRTOSTaskStartScheduler(void);

static ezSTATUS ezOsal_FreeRTOSSemaphoreCreate(ezOsal_SemaphoreHandle_t *handle);
static ezSTATUS ezOsal_SemaphoreFreeRTOSDelete(ezOsal_SemaphoreHandle_t *handle);
static ezSTATUS ezOsal_SemaphoreFreeRTOSTake(ezOsal_SemaphoreHandle_t *handle, uint32_t timeout_ticks);
static ezSTATUS ezOsal_SemaphoreFreeRTOSGive(ezOsal_SemaphoreHandle_t *handle);

static ezSTATUS ezOsal_FreeRTOSTimerCreate(ezOsal_TimerHandle_t *handle);
static ezSTATUS ezOsal_FreeRTOSTimerDelete(ezOsal_TimerHandle_t *handle);
static ezSTATUS ezOsal_FreeRTOSTimerStart(ezOsal_TimerHandle_t *handle);
static ezSTATUS ezOsal_FreeRTOSTimerStop(ezOsal_TimerHandle_t *handle);

ezSTATUS ezOsal_FreeRTOSEventCreate(ezOsal_EventHandle_t *handle);
ezSTATUS ezOsal_FreeRTOSEventDelete(ezOsal_EventHandle_t *handle);
int ezOsal_FreeRTOSEventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks);
ezSTATUS ezOsal_FreeRTOSEventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask);
ezSTATUS ezOsal_FreeRTOSEventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask);

static const ezOsal_Interfaces_t freertos_interface = {
    .TaskCreate = ezOsal_FreeRTOSTaskCreate,
    .TaskDelete = ezOsal_FreeRTOSTaskDelete,
    .TaskSuspend = ezOsal_FreeRTOSTaskSuspend,
    .TaskResume = ezOsal_FreeRTOSTaskResume,
    .TaskDelay = ezOsal_FreeRTOSTaskDelay,
    .TaskGetTickCount = ezOsal_FreeRTOSTaskGetTickCount,
    .TaskStartScheduler = ezOsal_FreeRTOSTaskStartScheduler,

    .SemaphoreCreate = ezOsal_FreeRTOSSemaphoreCreate,
    .SemaphoreDelete = ezOsal_SemaphoreFreeRTOSDelete,
    .SemaphoreTake = ezOsal_SemaphoreFreeRTOSTake,
    .SemaphoreGive = ezOsal_SemaphoreFreeRTOSGive,

    .TimerCreate = ezOsal_FreeRTOSTimerCreate,
    .TimerDelete = ezOsal_FreeRTOSTimerDelete,
    .TimerStart = ezOsal_FreeRTOSTimerStart,
    .TimerStop = ezOsal_FreeRTOSTimerStop,

    .EventCreate = ezOsal_FreeRTOSEventCreate,
    .EventDelete = ezOsal_FreeRTOSEventDelete,
    .EventWait = ezOsal_FreeRTOSEventWait,
    .EventSet = ezOsal_FreeRTOSEventSet,
    .EventClear = ezOsal_FreeRTOSEventClear
};


/*****************************************************************************
* Public functions
*****************************************************************************/
const ezOsal_Interfaces_t *ezOsal_FreeRTOSGetInterface(void)
{
    return (const ezOsal_Interfaces_t *)&freertos_interface;
}


/*****************************************************************************
* Local functions
*****************************************************************************/
ezSTATUS ezOsal_FreeRTOSTaskCreate(ezOsal_TaskHandle_t* handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTaskCreate(task_name = %s, stack_size = %d, priority = %d)",
            handle->task_name,
            handle->stack_size,
            handle->priority);

#if (EZ_OSAL_USE_STATIC == 1)
        ASSERT_MSG(handle->static_resource != NULL, "static resource must be set");
        ASSERT_MSG(((ezOsal_TaskResource_t*)handle->static_resource)->stack != NULL,
            "static resource must be set");

        handle->task_handle = xTaskCreateStatic(handle->task_function,
            handle->task_name,
            handle->stack_size,
            handle->argument,
            handle->priority,
            ((ezOsal_TaskResource_t*)handle->static_resource)->stack,
            &((ezOsal_TaskResource_t*)handle->static_resource)->task_block);
        if(handle->task_handle != NULL)
        {
            return ezSUCCESS;
        }
#else
        if(xTaskCreate(handle->task_function,
            handle->task_name, 
            handle->stack_size,
            handle->argument,
            handle->priority,
            (TaskHandle_t*)&handle->task_handle) == pdPASS)
        {
            return ezSUCCESS;
        }
#endif /* (EZ_OSAL_USE_STATIC == 1) */
    }
    EZERROR("Task create failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTaskDelete(ezOsal_TaskHandle_t* handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTaskDelete()");
        vTaskDelete((TaskHandle_t)handle->task_handle);
        return ezSUCCESS;
    }
    EZERROR("Task delete failed");
    return ezFAIL;
}


static ezSTATUS ezOsal_FreeRTOSTaskSuspend(ezOsal_TaskHandle_t* handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTaskSuspend()");
        vTaskSuspend((TaskHandle_t)handle->task_handle);
        return ezSUCCESS;
    }
    EZERROR("Task suspend failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTaskResume(ezOsal_TaskHandle_t* handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTaskResume()");
        vTaskResume((TaskHandle_t)handle->task_handle);
        return ezSUCCESS;
    }
    EZERROR("Task resume failed");
    return ezFAIL;
}


static ezSTATUS ezOsal_FreeRTOSTaskDelay(unsigned long num_of_ticks)
{
    EZTRACE("ezOsal_FreeRTOSTaskDelay()");
    vTaskDelay(num_of_ticks);
    return ezSUCCESS;
}


static unsigned long ezOsal_FreeRTOSTaskGetTickCount(void)
{
    EZTRACE("ezOsal_FreeRTOSTaskGetTickCount()");
    return xTaskGetTickCount();
}


static void ezOsal_FreeRTOSTaskStartScheduler(void)
{
    EZTRACE("ezOsal_FreeRTOSTaskStartScheduler()");
    vTaskStartScheduler();
}

static ezSTATUS ezOsal_FreeRTOSSemaphoreCreate(ezOsal_SemaphoreHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSSemaphoreCreate(max_count = %d)", handle->max_count);
#if (EZ_OSAL_USE_STATIC == 1)
        ASSERT_MSG(handle->static_resource != NULL, "static_resource must be set");
        handle->handle = xSemaphoreCreateCountingStatic(handle->max_count, 0, (StaticSemaphore_t*)handle->static_resource);
#else
        handle->handle = xSemaphoreCreateCounting(handle->max_count, 0); 
#endif
        if(handle->handle != NULL)
        {
            return ezSUCCESS;
        }
    }
    EZERROR("Semaphore create failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_SemaphoreFreeRTOSDelete(ezOsal_SemaphoreHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_SemaphoreFreeRTOSDelete()");
        vSemaphoreDelete((SemaphoreHandle_t)handle->handle);
        return ezSUCCESS;
    }
    EZERROR("Semaphore delete failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_SemaphoreFreeRTOSTake(ezOsal_SemaphoreHandle_t *handle, uint32_t timeout_ticks)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_SemaphoreFreeRTOSTake()");
        if(xSemaphoreTake((SemaphoreHandle_t)handle->handle, timeout_ticks) == pdTRUE)
        {
            return ezSUCCESS;
        }
    }
    EZWARNING("Semaphore is taken");
    return ezFAIL;
}

static ezSTATUS ezOsal_SemaphoreFreeRTOSGive(ezOsal_SemaphoreHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_SemaphoreFreeRTOSGive()");
        xSemaphoreGive((SemaphoreHandle_t)handle->handle);
        return ezSUCCESS;
    }
    EZERROR("Semaphore give failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTimerCreate(ezOsal_TimerHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTimerCreate(name = %s, period_ticks = %d)",
            handle->timer_name, handle->period_ticks);
    
            handle->handle = xTimerCreate(handle->timer_name,
                handle->period_ticks,
                pdTRUE,
                handle->argument,
                (TimerCallbackFunction_t)handle->timer_callback);
        if(handle->handle != NULL)
        {
            return ezSUCCESS;
        }
    }
    EZERROR("Timer create failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTimerDelete(ezOsal_TimerHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTimerDelete()");
        xTimerDelete(handle->handle, 0);
        return ezSUCCESS;
    }
    EZERROR("Timer delete failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTimerStart(ezOsal_TimerHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTimerStart()");
        if(xTimerStart(handle->handle, 0) == pdPASS)
        {
            return ezSUCCESS;
        }
    }
    EZERROR("Timer start failed");
    return ezFAIL;
}

static ezSTATUS ezOsal_FreeRTOSTimerStop(ezOsal_TimerHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSTimerStop()");
        if(xTimerStop(handle->handle, 0) == pdPASS)
        {
            return ezSUCCESS;
        }
    }
    EZERROR("Timer stop failed");
    return ezFAIL;
}

ezSTATUS ezOsal_FreeRTOSEventCreate(ezOsal_EventHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSEventCreate()");
#if (EZ_OSAL_USE_STATIC == 1)
        ASSERT_MSG(handle->static_resource != NULL, "static_resource must be set");
        handle->handle = (void*)xEventGroupCreateStatic((StaticEventGroup_t*)handle->static_resource);
#else
        handle->handle = (void*)xEventGroupCreate();
#endif
        if(handle->handle != NULL)
        {
            return ezSUCCESS;
        }
    }
    EZERROR("Event create failed");
    return ezFAIL;
}

ezSTATUS ezOsal_FreeRTOSEventDelete(ezOsal_EventHandle_t *handle)
{
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSEventDelete()");
        vEventGroupDelete((EventGroupHandle_t)handle->handle);
        return ezSUCCESS;
    }
    EZERROR("Event delete failed");
    return ezFAIL;
}

int ezOsal_FreeRTOSEventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks)
{
    EventBits_t bits = 0;
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSEventWait()");
        bits = xEventGroupWaitBits((EventGroupHandle_t)handle->handle, event_mask, pdTRUE, pdFALSE, timeout_ticks);
        EZDEBUG("events set = %d, event mask = %d", event_mask, bits);
        if((bits & event_mask) > 0)
        {
            EZDEBUG("event is set");
            return bits;
        }
    }
    EZDEBUG("Event = %d timeout", event_mask);
    return bits;
}

ezSTATUS ezOsal_FreeRTOSEventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    EventBits_t bits = 0;
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSEventSet()");
        EZDEBUG("Set event mask = %d", event_mask);
        bits = xEventGroupSetBits((EventGroupHandle_t)handle->handle, event_mask);
        if((bits & event_mask) == event_mask)
        {
            EZDEBUG("Set event success");
            return ezSUCCESS;
        }
    }
    EZERROR("Event set failed");
    return ezFAIL;
}

ezSTATUS ezOsal_FreeRTOSEventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    EventBits_t bits = 0;
    if(handle != NULL)
    {
        EZTRACE("ezOsal_FreeRTOSEventClear()");
        EZDEBUG("Clear event mask = %d", event_mask);
        bits = xEventGroupClearBits((EventGroupHandle_t)handle->handle, event_mask);
        if((bits & event_mask) == event_mask)
        {
            EZDEBUG("Clear event success");
            return ezSUCCESS;
        }
        return ezSUCCESS;
    }
    EZERROR("Event clear failed");
    return ezFAIL;
}

#endif /* EZ_FREERTOS_PORT == 1 */
/* End of file*/
