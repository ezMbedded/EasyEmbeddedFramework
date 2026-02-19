/*****************************************************************************
* Filename:         ez_task_worker.h
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

/** @file   ez_task_worker.h
 *  @author Hai Nguyen
 *  @date   29.03.2024
 *  @brief  Public API of task worker component
 *
 *  @details -
 */

#ifndef _EZ_TASK_WORKER_H
#define _EZ_TASK_WORKER_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_TASK_WORKER == 1)
#include "ez_linked_list.h"
#include "ez_queue.h"

#if (EZ_OSAL == 1)
#include "ez_osal.h"
#endif


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define EZ_THREAD_WAIT_NO       0x00        /* Thread does not wait for event, semaphore */
#define EZ_THREAD_WAIT_FOREVER  0xFFFFFFFF  /* Thread waits for event, semaphore forever */
#define EZ_EVENT_TASK_AVAIL     0x01        /* Task avaialble event */

#define INIT_WORKER(name, worker_sleep_ticks) \
    struct ezTaskWorker name =\
    {\
        .worker_name = #name,\
        .sleep_ticks = worker_sleep_ticks,\
    }



/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief definition of an ezTaskWorker
 */
struct ezTaskWorker
{

    ezQueue msg_queue;                      /**< Queue containing the tasks to be executed */
    char* worker_name;                      /**< Name of the worker */
    uint32_t sleep_ticks;                   /**< Number of tick the thread must sleep before being activated again */
#if ((EZ_FREERTOS_PORT == 1) || (EZ_THREADX_PORT == 1))
    ezOsal_TaskHandle_t *task_handle;       /**< task handle */
    ezOsal_SemaphoreHandle_t *sem_handle;   /**< semaphore handle */
    ezOsal_EventHandle_t *event_handle;     /**< event handle */
#else
    struct Node node;               /**< Linked list node */
#endif /* (EZ_OSAL == 1) */
};


/** @brief Definition of callback function to notify a task is finished or
 *         error occurs
 *  @param[out] event: retrun event, depending on the application
 *  @param[out] ret_data: return data, depending on the task
 *  @return: true if a task is executed successfully, otherwise false
 */
typedef void (*ezTaskWorkerCallbackFunc)(uint8_t event, void *ret_data);


/** @brief Definition of task function.
 *  @param[in] context: context containing data of a task
 *  @param[in] callback: callback to notify the caller about the status of the task
 *  @return: true if a task is executed successfully, otherwise false
 */
typedef bool (*ezTaskWorkerTaskFunc)(void *context, ezTaskWorkerCallbackFunc callback);


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezTaskWorker_CreateWorker
*//** 
* @brief Create a Task Worker
*
* @details This function create the task queue, add the worker to list
*          of worker for managing. If an RTOS is activated, it will create a
*          thread, a semephore, and an event group of the worker.
*
* @param[in]    worker: Worker to be initialized
* @param[in]    queue_buffer: buffer to queue task and data
* @param[in]    queue_buffer_size; size of the buffer
* @return       Return true if success, otherwise false
*
* @pre None
* @post None
*
* \b Example
* @code
*
* uint8_t queue_buff[128];
* struct ezTaskWorker worker;
*
* bool bResult = ezTaskWorker_InitializeWorker(&worker, queue_buff, 128);
* @endcode
*
* @see
*
*****************************************************************************/
bool ezTaskWorker_CreateWorker(struct ezTaskWorker *worker,
                               uint8_t *queue_buffer,
                               uint32_t queue_buffer_size);


/*****************************************************************************
* Function: ezTaskWorker_EnqueueTask
*//** 
* @brief Enqueue a task and its data so that they can be executed later
*
* @details
*
* @param[in]    worker: worker that will execute the task
* @param[in]    task: pointing to the task function
* @param[in]    callback: callback to return result of the task's execution.
* @param[in]    context: context data of the task function. Normally it contains
*                        the task's function parameters
* @param[in]    contex_size: size of the context
* @return       Return true if success, otherwise false
*
* @pre None
* @post None
*
* \b Example
* @code
*
* struct Context context;
* struct ezTaskWorker worker;
*
* bool bResult = ezTaskWorker_EnqueueTask(&worker, worker_task, callback, &context, sizeof(struct Context));
* @endcode
*
* @see
*
*****************************************************************************/
bool ezTaskWorker_EnqueueTask(struct ezTaskWorker *worker,
                              ezTaskWorkerTaskFunc task,
                              ezTaskWorkerCallbackFunc callback,
                              void *context,
                              uint32_t context_size,
                              uint32_t ticks_to_wait);

#if ((EZ_THREADX_PORT == 1) || (EZ_FREERTOS_PORT == 1))
/*****************************************************************************
* Function: ezTaskWorker_ExecuteTask
*//** 
* @brief This function is call within the THREAD_FUNC to let the worker execute
*        available task
*
* @details This function is used when RTOS is activated
*
* @param[in]    worker: pointer to the worker which execute the task
* @param[in]    ticks_to_wait: number of tick to wait for task available
*               event (EZ_EVENT_TASK_AVAIL) and to wait for semaphore available.
*               Use EZ_THREAD_WAIT_NO to not to wait or EZ_THREAD_WAIT_FOREVER to
*               wait unti the event and semaphore available
* @return       None
*
* @pre worker is created by ezTaskWorker_CreateWorker and RTOS interface is set
* @post None
*
* \b Example
* @code
*
* THREAD_FUNC(worker1)
* {
*     ezTaskWorker_ExecuteTask(&worker1, EZ_THREAD_WAIT_FOREVER);
* }
*
* @endcode
*
* @see ezTaskWorker_CreateWorker, ezTaskWorker_SetRtosInterface
*
*****************************************************************************/
void ezTaskWorker_ExecuteTask(struct ezTaskWorker *worker, uint32_t ticks_to_wait);
#else
/*****************************************************************************
* Function: ezTaskWorker_ExecuteTaskNoRTOS
*//** 
* @brief If no RTOS is used, this function provides the processing time to
*        the worker
*
* @details This function must be called periodically to provide processing time
*
* @param    None
* @return   None
*
* @pre None
* @post None
*
* \b Example
* @code
*
* //super loop
* while(1)
* {
*     ezTaskWorker_ExecuteTaskNoRTOS();
* }
*
* @endcode
*
* @see
*
*****************************************************************************/
void ezTaskWorker_ExecuteTaskNoRTOS(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* EZ_TASK_WORKER == 1 */
#endif /* _EZ_TASK_WORKER_H */


/* End of file */
