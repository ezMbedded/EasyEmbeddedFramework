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
#include "ez_osal_threadx.h"
#endif

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define BUFF_SIZE   256
#define PRIORITY    10
#define STACK_SIZE  512

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static uint8_t buff1[BUFF_SIZE] = {0};

static INIT_WORKER(worker1, 100);

static ezOsal_TaskResource_t task1_resource;
static ezOsal_SemaphoreResource_t semaphore1_resource;
static ezOsal_EventResource_t event1_resource;

static void worker1_thread(void *arg);

static EZ_OSAL_DEFINE_TASK_HANDLE(worker1_task, STACK_SIZE, 1, worker1_thread, NULL, &task1_resource);
static EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(worker1_semaphore, 1, &semaphore1_resource);
static EZ_OSAL_DEFINE_EVENT_HANDLE(worker1_event, &event1_resource);

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static bool worker1_sum_intern(void *context, ezTaskWorkerCallbackFunc callback);


/*****************************************************************************
* Public functions
*****************************************************************************/
void ezApp_Worker1Init(void)
{
    worker1.task_handle = &worker1_task;
    worker1.sem_handle = &worker1_semaphore;
    worker1.event_handle = &worker1_event;
    ezTaskWorker_CreateWorker(&worker1, buff1, BUFF_SIZE);
}


bool worker1_sum(Worker1_SumContext *context, ezTaskWorkerCallbackFunc callback)
{
    bool ret = false;

    ret = ezTaskWorker_EnqueueTask(&worker1,
                                   worker1_sum_intern,
                                   callback,
                                   (void*)context,
                                   sizeof(Worker1_SumContext),
                                   EZ_THREAD_WAIT_FOREVER);
    return ret;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static void worker1_thread(void *arg)
{
    while(1)
    {
        ezTaskWorker_ExecuteTask(&worker1, EZ_THREAD_WAIT_FOREVER);
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
