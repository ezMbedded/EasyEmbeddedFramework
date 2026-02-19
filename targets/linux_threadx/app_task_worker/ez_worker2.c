/*****************************************************************************
* Filename:         ez_worker2.c
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

/** @file   ez_worker2.c
 *  @author Hai Nguyen
 *  @date   30.03.2025
 *  @brief  Worker 2 application
 *
 *  @details -
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_worker2.h"


#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_worker2"       /**< module name */
#include "ez_logging.h"

#if(EZ_OSAL == 1)
#include "ez_osal.h"
#include "ez_osal_threadx.h"
#endif

#include "ez_task_worker.h"
#include "ez_worker1.h"

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
static uint8_t buff2[BUFF_SIZE] = {0};
static INIT_WORKER(worker2, 200);

static void worker2_thread(void *arg);

static ezOsal_TaskResource_t task2_resource;
static ezOsal_SemaphoreResource_t semaphore2_resource;
static ezOsal_EventResource_t event2_resource;

static EZ_OSAL_DEFINE_TASK_HANDLE(worker2_task, STACK_SIZE, 1, worker2_thread, NULL,  &task2_resource);
static EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(worker2_semaphore, 1, &semaphore2_resource);
static EZ_OSAL_DEFINE_EVENT_HANDLE(worker2_event, &event2_resource);

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void worker2_callback(uint8_t event, void *ret_data);

/*****************************************************************************
* Public functions
*****************************************************************************/
void ezApp_Worker2Init(void)
{
    worker2.task_handle = &worker2_task;
    worker2.sem_handle = &worker2_semaphore;
    worker2.event_handle = &worker2_event;
    ezTaskWorker_CreateWorker(&worker2, buff2, BUFF_SIZE);
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static void worker2_thread(void *arg)
{
    while(1)
    {
        Worker1_SumContext context;
        context.a = rand() % 255;
        context.b = rand() % 255;
        if(worker1_sum(&context, worker2_callback) == true)
        {
            EZINFO("Call sum service success");
        }
        ezOsal_TaskDelay(worker2.sleep_ticks);
    }
}


static void worker2_callback(uint8_t event, void *ret_data)
{
    switch (event)
    {
    case 0:
        if(ret_data != NULL)
        {
            EZINFO("sum = %d", *(int*)ret_data);
        }
        break;
    
    default:
        break;
    }
}


/* End of file*/
