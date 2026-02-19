/*****************************************************************************
* Filename:         ez_task_worker.c
* Author:           Hai Nguyen
* Original Date:    29.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_task_worker.c
 *  @author Hai Nguyen
 *  @date   29.03.2024
 *  @brief  Implementation of the task worker component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_TASK_WORKER == 1)

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_TASK_WORKER_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_task_worker"       /**< module name */
#include "ez_logging.h"
#include "ez_utilities_common.h"
#include "ez_task_worker.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None*/


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief definition of a task block
 */
typedef ezReservedElement ezTaskBlock_t;


/** @brief definition of common data that every task block must have.
 */
struct ezTaskBlockCommon
{
    ezTaskWorkerTaskFunc task;          /* Task that will be executed by the worker */
    ezTaskWorkerCallbackFunc callback;  /* Callback function to notify the result of the task execution */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static struct Node worker_list = EZ_LINKEDLIST_INIT_NODE(worker_list);
static struct ezTaskWorkerThreadInterfaces *rtos_interfaces = NULL;

/*****************************************************************************
* Function Definitions
*****************************************************************************/

/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezTaskWorker_CreateWorker(struct ezTaskWorker *worker,
                               uint8_t *queue_buffer,
                               uint32_t queue_buffer_size)
{
    EZTRACE("ezTaskWorker_CreateWorker()");
    ezSTATUS status = ezFAIL;

    if(worker != NULL)
    {
        #if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
        if(worker->task_handle != NULL && worker->event_handle != NULL && worker->sem_handle != NULL)
        {
            status = ezOsal_TaskCreate(worker->task_handle);
            
            if(status == ezSUCCESS)
            {
                status = ezOsal_EventCreate(worker->event_handle);
            }

            if(status == ezSUCCESS)
            {
                status = ezOsal_SemaphoreCreate(worker->sem_handle);
            }

            if(status == ezFAIL)
            {
                (void)ezOsal_TaskDelete(worker->task_handle);
                (void)ezOsal_EventDelete(worker->event_handle);
                (void)ezOsal_SemaphoreDelete(worker->sem_handle);
                EZERROR("Cannot create task, event or semaphore");
                return false;
            }
        }
        #else
        ezLinkedList_InitNode(&worker->node);
        if( EZ_LINKEDLIST_ADD_TAIL(&worker_list, &worker->node) == false)
        {
            EZERROR("Cannot add worker to list");
            return false;
        }
        #endif

        if(ezQueue_CreateQueue(&worker->msg_queue, queue_buffer, queue_buffer_size) != ezSUCCESS)
        {
            EZERROR("Cannot create queue");
            return false;
        }
    }
    return true;
}


bool ezTaskWorker_EnqueueTask(struct ezTaskWorker *worker,
                              ezTaskWorkerTaskFunc task,
                              ezTaskWorkerCallbackFunc callback,
                              void *context,
                              uint32_t context_size,
                              uint32_t ticks_to_wait)
{
    bool ret = false;
    void *buff = NULL;
    ezTaskBlock_t task_block = NULL;
    ezSTATUS status = ezFAIL;

    EZTRACE("ezTaskWorker_EnqueueTask()");

    if((worker != NULL) && (task != NULL) && (callback != NULL))
    {
        ret = true;

        #if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
        EZTRACE("Getting semaphore from worker = %s", worker->worker_name);
        status = ezOsal_SemaphoreTake(worker->sem_handle, ticks_to_wait);
        if(status != ezSUCCESS)
        {
            ret = false;
        }
        #endif /* (EZ_OSAL == 1) */

        if(ret == true)
        {
            /**The idea to store common data and context data is we reserve a buffer
             * with the size = common size + context size. Then, the buffer is convert to
             * ezTaskBlockCommon to store common data. After that it is offseted to the
             * address = common address + sizeof(ezTaskBlockCommon). Then the context
             * is copied to that address.
             */
            task_block = (ezTaskBlock_t)ezQueue_ReserveElement(&worker->msg_queue,
                                                               &buff,
                                                               sizeof(struct ezTaskBlockCommon) + context_size);
            if((task_block != NULL) && (buff != NULL))
            {
                /* Set common data */
                ((struct ezTaskBlockCommon*)buff)->callback = callback;
                ((struct ezTaskBlockCommon*)buff)->task = task;

                /* Offset the pointer */
                buff += sizeof(struct ezTaskBlockCommon);

                /* Copy context data */
                memcpy(buff, context, context_size);
                status = ezQueue_PushReservedElement(&worker->msg_queue,
                                                    (ezReservedElement)task_block);
                if(status == ezSUCCESS)
                {
                    ret = true;
                    EZINFO("Add new task to %s",worker->worker_name);

                    #if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
                    if(ezOsal_EventSet(worker->event_handle, EZ_EVENT_TASK_AVAIL) != ezSUCCESS)
                    {
                        ret = false;
                    }
                    #endif /* ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1)) */
                }
                else
                {
                    ret = false;
                    ezQueue_ReleaseReservedElement(&worker->msg_queue,
                                                   (ezReservedElement)task_block);
                    EZERROR("Cannot add task to %s",worker->worker_name);
                }
            }
#if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
            /* Expect nothing wrong when giving semaphore */
            ezOsal_SemaphoreGive(worker->sem_handle);
#endif /* ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1)) */
        }
    }

    if(ret == false)
    {
        EZERROR("Enqueue task error");
    }

    return ret;
}

#if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
void ezTaskWorker_ExecuteTask(struct ezTaskWorker *worker, uint32_t ticks_to_wait)
{
    ezSTATUS status = ezFAIL;
    void *context = NULL;
    struct ezTaskBlockCommon *common = NULL;
    uint32_t data_size = 0;

    if(worker != NULL)
    {
        EZTRACE("ezTaskWorker_ExecuteTask(woker = %s)", worker->worker_name);

        status = ezOsal_EventWait(worker->event_handle, EZ_EVENT_TASK_AVAIL, ticks_to_wait);

        if(status == ezSUCCESS)
        {
            EZDEBUG("Receive EZ_EVENT_TASK_AVAIL");
            EZTRACE("Getting semaphore from %s", worker->worker_name);
            status = ezOsal_SemaphoreTake(worker->sem_handle, ticks_to_wait);
        }
        else
        {
            EZDEBUG("Receive event error or timeout");
        }

        if(status == ezSUCCESS)
        {
            EZTRACE("Got semaphore from worker = %s", worker->worker_name);
            status = ezQueue_GetFront(&worker->msg_queue, (void**)&common, &data_size);

            if((status == ezSUCCESS) && (common->task != NULL))
            {
                context = common;
                context += sizeof(struct ezTaskBlockCommon);
                common->task(context, common->callback);
                status = ezQueue_PopFront(&worker->msg_queue);
            }

            (void)ezOsal_SemaphoreGive(worker->sem_handle);
        }
        else
        {
            EZDEBUG("Cannot get semaphore within %d ticks or error", ticks_to_wait);
        }
    }
}
#else
void ezTaskWorker_ExecuteTaskNoRTOS(void)
{
    struct Node *it = NULL;
    struct ezTaskBlockCommon *common = NULL;
    void *context = NULL;
    struct ezTaskWorker *worker = NULL;
    ezSTATUS status = ezFAIL;
    uint32_t data_size = 0;

    EZTRACE("ezTaskWorker_Run()");

    EZ_LINKEDLIST_FOR_EACH(it, &worker_list)
    {
        worker = EZ_LINKEDLIST_GET_PARENT_OF(it, node, struct ezTaskWorker);
        if(ezQueue_GetNumOfElement(&worker->msg_queue) > 0)
        {
            status = ezQueue_GetFront(&worker->msg_queue, (void**)&common, &data_size);
            if(status == ezSUCCESS && common->task != NULL)
            {
                context = common;
                context += sizeof(struct ezTaskBlockCommon);
                common->task(context, common->callback);
            }
            status = ezQueue_PopFront(&worker->msg_queue);
        }
    }
}
#endif /* (EZ_THREADX_PORT == 1) || (EZ_FREERTOS_PORT == 1) */

/*****************************************************************************
* Local functions
*****************************************************************************/


#endif /* (EZ_OSAL == 1) */
/* End of file*/
