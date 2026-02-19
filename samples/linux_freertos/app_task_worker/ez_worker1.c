/*****************************************************************************
* Filename:         ez_worker1.c
* Author:           Hai Nguyen
* Original Date:    30.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_worker1.c
 *  @author Hai Nguyen
 *  @date   30.03.2025
 *  @brief  Worker 1 application
 *
 *  @details -
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_worker1.h"


#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_worker1"       /**< module name */
#include "ez_logging.h"

#if(EZ_OSAL == 1)
#include "ez_osal.h"
#include "ez_osal_freertos.h"
#endif

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define BUFF_SIZE   256
#define PRIORITY    10
#define STACK_SIZE  256

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static uint8_t buff1[BUFF_SIZE] = {0};
static INIT_WORKER(worker1, 10);

static void worker1_thread(void *arg);


#if (EZ_OSAL_USE_STATIC == 1)
static ezOsal_Stack_t stack1[STACK_SIZE];
static ezOsal_TaskResource_t task1_resource;
static ezOsal_SemaphoreResource_t semaphore_resource;
static ezOsal_EventResource_t event_resource;

static EZ_OSAL_DEFINE_TASK_HANDLE(worker1_task, STACK_SIZE, 1, worker1_thread, NULL, &task1_resource);
static EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(worker1_semaphore, 1, &semaphore_resource);
static EZ_OSAL_DEFINE_EVENT_HANDLE(worker1_event, &event_resource);
#else
static EZ_OSAL_DEFINE_TASK_HANDLE(worker1_task, STACK_SIZE, 1, worker1_thread, NULL, NULL);
static EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(worker1_semaphore, 1, NULL);
static EZ_OSAL_DEFINE_EVENT_HANDLE(worker1_event, NULL);
#endif

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static bool worker1_sum_intern(void *context, ezTaskWorkerCallbackFunc callback);


/*****************************************************************************
* Public functions
*****************************************************************************/
void ezApp_Worker1Init(void)
{
#if (EZ_OSAL_USE_STATIC == 1)
    task1_resource.stack = stack1;
#endif
    worker1.task_handle = &worker1_task;
    worker1.sem_handle = &worker1_semaphore;
    worker1.event_handle = &worker1_event;
    ezTaskWorker_CreateWorker(&worker1, buff1, BUFF_SIZE);
    ezOsal_SemaphoreGive(&worker1_semaphore);
}


bool worker1_sum(Worker1_SumContext *context, ezTaskWorkerCallbackFunc callback)
{
    bool ret = false;
    EZINFO("worker1_sum() is called");
    ret = ezTaskWorker_EnqueueTask(&worker1,
                                   worker1_sum_intern,
                                   callback,
                                   (void*)context,
                                   sizeof(Worker1_SumContext),
                                   10);
    return ret;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static void worker1_thread(void *arg)
{
    while(1)
    {
        ezTaskWorker_ExecuteTask(&worker1, 10);
        ezOsal_TaskDelay(worker1.sleep_ticks);
    }
}


static bool worker1_sum_intern(void *context, ezTaskWorkerCallbackFunc callback)
{
    bool ret = false;
    int sum = 0;
    Worker1_SumContext *sum_context = (Worker1_SumContext *)context;
    if(sum_context != NULL && callback != NULL)
    {
        sum = sum_context->a + sum_context->b;
        callback(0, &sum);
        ret = true;
    }

    return true;
}

/* End of file*/
