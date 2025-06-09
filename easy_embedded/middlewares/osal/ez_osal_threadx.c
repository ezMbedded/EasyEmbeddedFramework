/*****************************************************************************
* Filename:         ez_osal_threadx.c
* Author:           Hai Nguyen
* Original Date:    21.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_osal_threadx.c
 *  @author Hai Nguyen
 *  @date   21.03.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_osal_threadx.h"

#if (EZ_THREADX_PORT == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_OSAL_THREADX_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_osal_threadx"       /**< module name */
#include "ez_logging.h"
#include "tx_api.h"
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
static TX_BYTE_POOL threadx_byte_pool;
static CHAR *mem_pointer = NULL;

/*****************************************************************************
* Function Definitions
*****************************************************************************/

static ezSTATUS ezOsal_ThreadXInit(void* argument);
static ezSTATUS ezOsal_ThreadXTaskCreate(ezOsal_TaskHandle_t* task_handle);
static ezSTATUS ezOsal_ThreadXTaskDelete(ezOsal_TaskHandle_t *task_handle);
static ezSTATUS ezOsal_ThreadXTaskSuspend(ezOsal_TaskHandle_t *task_handle);
static ezSTATUS ezOsal_ThreadXTaskResume(ezOsal_TaskHandle_t *task_handle);
static ezSTATUS ezOsal_ThreadXTaskDelay(unsigned long num_of_ticks);
static unsigned long ezOsal_ThreadXTaskGetTickCount(void);
static void ezOsal_ThreadXTaskStartScheduler(void);

static ezSTATUS ezOsal_ThreadXSemaphoreCreate(ezOsal_SemaphoreHandle_t *semaphore_handle);
static ezSTATUS ezOsal_SemaphoreThreadXDelete(ezOsal_SemaphoreHandle_t *semaphore_handle);
static ezSTATUS ezOsal_SemaphoreThreadXTake(ezOsal_SemaphoreHandle_t *semaphore_handle, uint32_t timeout_ticks);
static ezSTATUS ezOsal_SemaphoreThreadXGive(ezOsal_SemaphoreHandle_t *semaphore_handle);

static ezSTATUS ezOsal_ThreadXTimerCreate(ezOsal_TimerHandle_t *timer_handle);
static ezSTATUS ezOsal_ThreadXTimerDelete(ezOsal_TimerHandle_t *timer_handle);
static ezSTATUS ezOsal_ThreadXTimerStart(ezOsal_TimerHandle_t *timer_handle);
static ezSTATUS ezOsal_ThreadXTimerStop(ezOsal_TimerHandle_t *timer_handle);

static ezSTATUS ezOsal_ThreadXEventCreate(ezOsal_EventHandle_t *handle);
static ezSTATUS ezOsal_ThreadXEventDelete(ezOsal_EventHandle_t *handle);
static int ezOsal_ThreadXOSEventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks);
static ezSTATUS ezOsal_ThreadXEventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask);
static ezSTATUS ezOsal_ThreadXEventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask);

static const ezOsal_Interfaces_t threadx_interface = {
    .Init = ezOsal_ThreadXInit,
    .TaskCreate = ezOsal_ThreadXTaskCreate,
    .TaskDelete = ezOsal_ThreadXTaskDelete,
    .TaskSuspend = ezOsal_ThreadXTaskSuspend,
    .TaskResume = ezOsal_ThreadXTaskResume,
    .TaskDelay = ezOsal_ThreadXTaskDelay,
    .TaskGetTickCount = ezOsal_ThreadXTaskGetTickCount,
    .TaskStartScheduler = ezOsal_ThreadXTaskStartScheduler,

    .SemaphoreCreate = ezOsal_ThreadXSemaphoreCreate,
    .SemaphoreDelete = ezOsal_SemaphoreThreadXDelete,
    .SemaphoreTake = ezOsal_SemaphoreThreadXTake,
    .SemaphoreGive = ezOsal_SemaphoreThreadXGive,

    .TimerCreate = ezOsal_ThreadXTimerCreate,
    .TimerDelete = ezOsal_ThreadXTimerDelete,
    .TimerStart = ezOsal_ThreadXTimerStart,
    .TimerStop = ezOsal_ThreadXTimerStop,

    .EventCreate = ezOsal_ThreadXEventCreate,
    .EventDelete = ezOsal_ThreadXEventDelete,
    .EventWait = ezOsal_ThreadXOSEventWait,
    .EventSet = ezOsal_ThreadXEventSet,
    .EventClear = ezOsal_ThreadXEventClear,
};

static void ezOsal_ThreadXPrintStatusCode(UINT code);


/*****************************************************************************
* Public functions
*****************************************************************************/
const ezOsal_Interfaces_t *ezOsal_ThreadXGetInterface(void)
{
    return &threadx_interface;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static ezSTATUS ezOsal_ThreadXInit(void* argument)
{
    UINT threadx_status = TX_THREAD_ERROR;

    threadx_status = tx_byte_pool_create(&threadx_byte_pool, NULL, argument, 4096);
    
    if(threadx_status == TX_SUCCESS)
    {
        EZINFO("Initialization success");
        return ezSUCCESS;
    }

    ezOsal_ThreadXPrintStatusCode(threadx_status);
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTaskCreate(ezOsal_TaskHandle_t* task_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(task_handle != NULL)
    {
        ASSERT_MSG(task_handle->static_resource != NULL,
            "task_handle->static_resource is null, please set it to ezOsal_TaskResource_t");
        status = tx_byte_allocate(&threadx_byte_pool, 
            (void**)&mem_pointer,
            task_handle->stack_size,
            TX_NO_WAIT);

        if(status == TX_SUCCESS)
        {
            status = tx_thread_create((TX_THREAD*)task_handle->static_resource,
                (CHAR *)task_handle->task_name,
                (VOID (*)(ULONG))task_handle->task_function,
                (ULONG)((uintptr_t)task_handle->argument),
                mem_pointer,
                task_handle->stack_size,
                task_handle->priority,
                task_handle->priority,
                TX_NO_TIME_SLICE,
                TX_AUTO_START);

            if(status == TX_SUCCESS)
            {
                return ezSUCCESS;
            }
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTaskDelete(ezOsal_TaskHandle_t *task_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(task_handle != NULL)
    {
        status = tx_thread_terminate((TX_THREAD*)task_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            status = tx_thread_delete((TX_THREAD*)task_handle->static_resource);
            if(status == TX_SUCCESS)
            {
                task_handle->static_resource = NULL;
                return ezSUCCESS;
            }
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTaskSuspend(ezOsal_TaskHandle_t *task_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(task_handle != NULL)
    {
        status = tx_thread_suspend((TX_THREAD*)task_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTaskResume(ezOsal_TaskHandle_t *task_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(task_handle->static_resource != NULL)
    {
        status = tx_thread_resume(task_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTaskDelay(unsigned long num_of_ticks)
{
    tx_thread_sleep(num_of_ticks);
    return ezFAIL;
}


static unsigned long ezOsal_ThreadXTaskGetTickCount(void)
{
    return 0;
}


static void ezOsal_ThreadXTaskStartScheduler(void)
{
    tx_kernel_enter();
}


static ezSTATUS ezOsal_ThreadXSemaphoreCreate(ezOsal_SemaphoreHandle_t* semaphore_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(semaphore_handle != NULL)
    {
        ASSERT_MSG(semaphore_handle->static_resource != NULL,
            "semaphore_handle->static_resource is null, please set it to ezOsal_SemaphoreResource_t");
        status = tx_semaphore_create((TX_SEMAPHORE*)semaphore_handle->static_resource, (CHAR *)NULL, semaphore_handle->max_count);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_SemaphoreThreadXDelete(ezOsal_SemaphoreHandle_t* semaphore_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(semaphore_handle != NULL)
    {
        status = tx_semaphore_delete(semaphore_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            semaphore_handle = NULL;
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_SemaphoreThreadXTake(ezOsal_SemaphoreHandle_t* semaphore_handle,
    uint32_t timeout_ticks)
{
    UINT status = TX_THREAD_ERROR;
    if(semaphore_handle != NULL)
    {
        status = tx_semaphore_get(semaphore_handle->static_resource, timeout_ticks);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_SemaphoreThreadXGive(ezOsal_SemaphoreHandle_t* semaphore_handle)
{
    UINT status = TX_THREAD_ERROR;
    status = tx_semaphore_put(semaphore_handle->static_resource);
    if(status == TX_SUCCESS)
    {
        return ezSUCCESS;
    }
    ezOsal_ThreadXPrintStatusCode(status);
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTimerCreate(ezOsal_TimerHandle_t *timer_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(timer_handle != NULL)
    {
        ASSERT_MSG(timer_handle->static_resource != NULL,
            "timer_handle->static_resource is null, please set it to ezOsal_TimerResource_t");
        status = tx_timer_create((TX_TIMER*)timer_handle->static_resource,
            (CHAR *)timer_handle->timer_name,
            (VOID (*)(ULONG))timer_handle->timer_callback,
            (ULONG)((uintptr_t)timer_handle->argument),
            timer_handle->period_ticks,
            timer_handle->period_ticks,
            TX_NO_ACTIVATE);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTimerDelete(ezOsal_TimerHandle_t* timer_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(timer_handle != NULL)
    {
        status = tx_timer_delete(timer_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            timer_handle = NULL;
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXTimerStart(ezOsal_TimerHandle_t* timer_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(timer_handle != NULL)
    {
        status = tx_timer_activate(timer_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXEventCreate(ezOsal_EventHandle_t *handle)
{
    if(handle != NULL)
    {
        ASSERT_MSG(handle->static_resource != NULL,
            "handle->static_resource is null, please set it to ezOsal_EventResource_t");
        if(tx_event_flags_create((TX_EVENT_FLAGS_GROUP*)handle->static_resource, NULL) ==TX_SUCCESS)
        {
            return ezSUCCESS;
        }
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXEventDelete(ezOsal_EventHandle_t *handle)
{
    if(handle != NULL)
    {
        if(tx_event_flags_delete((TX_EVENT_FLAGS_GROUP*)handle->static_resource) == TX_SUCCESS)
        {
            handle->static_resource = NULL;
            return ezSUCCESS;
        }
    }
    return ezFAIL;
}


static int ezOsal_ThreadXOSEventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks)
{
    ULONG actual_events;
    UINT status;
    if(handle != NULL)
    {
        status = tx_event_flags_get((TX_EVENT_FLAGS_GROUP*)handle->static_resource, event_mask, TX_OR, &actual_events, timeout_ticks);
        if(status == TX_SUCCESS)
        {
            return actual_events;
        }
    }
    return 0;
}


static ezSTATUS ezOsal_ThreadXEventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    if(handle != NULL)
    {
        if(tx_event_flags_set((TX_EVENT_FLAGS_GROUP*)handle->static_resource, event_mask, TX_OR) == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
    }
    return ezFAIL;
}


static ezSTATUS ezOsal_ThreadXEventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    if(handle != NULL)
    {
        if(tx_event_flags_set((TX_EVENT_FLAGS_GROUP*)handle->static_resource, ~event_mask, TX_AND) == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
    }
    return ezFAIL;
}



static ezSTATUS ezOsal_ThreadXTimerStop(ezOsal_TimerHandle_t* timer_handle)
{
    UINT status = TX_THREAD_ERROR;
    if(timer_handle != NULL)
    {
        status = tx_timer_deactivate(timer_handle->static_resource);
        if(status == TX_SUCCESS)
        {
            return ezSUCCESS;
        }
        ezOsal_ThreadXPrintStatusCode(status);
    }
    return ezFAIL;
}


static void ezOsal_ThreadXPrintStatusCode(UINT code)
{
    switch(code)
    {
        case TX_POOL_ERROR:         EZERROR("TX_POOL_ERROR"); break;
        case TX_PTR_ERROR:          EZERROR("TX_PTR_ERROR"); break;
        case TX_SIZE_ERROR:         EZERROR("TX_SIZE_ERROR"); break;
        case TX_CALLER_ERROR:       EZERROR("TX_CALLER_ERROR"); break;
        case TX_GROUP_ERROR:        EZERROR("TX_GROUP_ERROR"); break;
        case TX_DELETED:            EZERROR("TX_DELETED"); break;
        case TX_NO_MEMORY:          EZERROR("TX_NO_MEMORY"); break;
        case TX_WAIT_ABORTED:       EZERROR("TX_WAIT_ABORTED"); break;
        case TX_WAIT_ERROR:         EZERROR("TX_WAIT_ERROR"); break;
        case TX_SEMAPHORE_ERROR:    EZERROR("TX_SEMAPHORE_ERROR"); break;
        case TX_OPTION_ERROR:       EZERROR("TX_OPTION_ERROR"); break;
        case TX_NOT_AVAILABLE:      EZERROR("TX_NOT_AVAILABLE "); break;
        case TX_MUTEX_ERROR:        EZERROR("TX_MUTEX_ERROR "); break;
        case TX_NO_INSTANCE:        EZERROR("TX_NO_INSTANCE "); break;
        default: EZERROR("Unknown code"); break;
    }
}

#endif /* EZ_OSAL_THREADX_ENABLE == 1 */
/* End of file*/
