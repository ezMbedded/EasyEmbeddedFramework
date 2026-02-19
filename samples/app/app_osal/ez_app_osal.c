/*****************************************************************************
* Filename:         ez_app_osal.c
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

/** @file   ez_app_osal.c
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Application to test OSAL component
 *
 *  @details None
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_app_osal.h"


#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_app_osal"       /**< module name */
#include "ez_logging.h"
#include "ez_osal.h"
#include "ez_osal_threadx.h"
#include "ez_osal_freertos.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define STACK_SIZE    256
#define FLAG0  (1 << 0)
#define FLAG1  (1 << 1)
#define FLAG2  (1 << 2)


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static const ezOsal_Interfaces_t *rtos_interface = NULL;


/*****************************************************************************
* Function Definitions
*****************************************************************************/
#if (EZ_FREERTOS_PORT == 1)
static ezOsal_Stack_t stack1[STACK_SIZE];
static ezOsal_Stack_t stack2[STACK_SIZE];
static ezOsal_Stack_t stack3[STACK_SIZE];

static ezOsal_TaskResource_t task1_resource = {
    .stack = stack1,
};
static ezOsal_TaskResource_t task2_resource = {
    .stack = stack2,
};
static ezOsal_TaskResource_t task3_resource = {
    .stack = stack3,
};

#elif (EZ_THREADX_PORT == 1)
static ezOsal_TaskResource_t task1_resource;
static ezOsal_TaskResource_t task2_resource;
static ezOsal_TaskResource_t task3_resource;
#else
#endif

static void Task1Function(void *argument);
static void Task2Function(void *argument);
static void Task3Function(void *argument);
static void TimerElapseCallback(void *argument);


static ezOsal_SemaphoreResource_t semaphore_resource;
static ezOsal_EventResource_t event_resource;
static ezOsal_TimerResource_t timer_resource;

EZ_OSAL_DEFINE_TASK_HANDLE(task1, STACK_SIZE, 1, Task1Function, NULL, &task1_resource);
EZ_OSAL_DEFINE_TASK_HANDLE(task2, STACK_SIZE, 1, Task2Function, NULL, &task2_resource);
EZ_OSAL_DEFINE_TASK_HANDLE(task3, STACK_SIZE, 1, Task3Function, NULL, &task3_resource);
EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(semaphore_handle, 1, &semaphore_resource);
EZ_OSAL_DEFINE_TIMER_HANDLE(timer, 50, TimerElapseCallback, NULL, &timer_resource);
EZ_OSAL_DEFINE_EVENT_HANDLE(event, &event_resource);

/*****************************************************************************
* Public functions
*****************************************************************************/
void ezApp_OsalInit(void* arg)
{
    (void) ezOsal_SemaphoreCreate(&semaphore_handle);
    (void) ezOsal_TimerCreate(&timer);

    //ezOsal_TimerStart(&timer);
    (void) ezOsal_TaskCreate(&task1);
    (void) ezOsal_TaskCreate(&task2);
    (void) ezOsal_TaskCreate(&task3);

    ezOsal_EventCreate(&event);

    ezOsal_TaskSuspend(&task1);
    ezOsal_TaskSuspend(&task2);
    ezOsal_TaskSuspend(&task3);
}

void ezApp_StartOsalApp(void)
{
    ezOsal_TimerStart(&timer);
    ezOsal_TaskResume(&task1);
    ezOsal_TaskResume(&task3);
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static void Task1Function(void *argument)
{
    uint8_t count = 0;
    ezSTATUS status = ezFAIL;

    EZINFO("Enter Task 1");
    while(1)
    {
        count++;
        EZINFO("Task 1 - count = %d", count);
        if(count == 5)
        {
            status = ezOsal_TaskResume(&task2);
            if(status == ezSUCCESS)
            {
                EZINFO("Resuming task 2");
            }
        }
        ezOsal_TaskDelay(10);
    }
}

static void Task2Function(void *argument)
{
    uint8_t count = 0;
    ezSTATUS status = ezFAIL;

    EZINFO("Enter Task 2");
    while(1)
    {
        count++;
        EZINFO("Task 2 - count = %d", count);
        if(count == 5)
        {
            status = ezOsal_TaskDelete(&task1);
            if(status == ezSUCCESS)
            {
                EZINFO("delete task 1");
            }
        }
        ezOsal_TaskDelay(20);
    }
}

static void TimerElapseCallback(void *argument)
{
    static int count = 0;
    ezSTATUS status = ezFAIL;

    EZINFO("Timer elapsed, count = %d", count);
    if(count == 2)
    {
        status = ezOsal_TaskDelete(&task2);
        if(status == ezSUCCESS)
        {
            EZINFO("delete task 2");
        }
    }
    else if (count == 4)
    {
        ezOsal_EventSet(&event, FLAG2);
    }

    if(count % 2 == 0)
    {
        ezOsal_EventSet(&event, FLAG0);
    }
    else
    {
        ezOsal_EventSet(&event, FLAG1);
    }

    count++;
}

static void Task3Function(void *argument)
{
    EZINFO("Enter Task 3");
    int bit_mask = 0;

    while(1)
    {
        bit_mask = ezOsal_EventWait(&event, (FLAG0 | FLAG1 | FLAG2), 10);
        
        if(bit_mask == 0)
        {
            EZINFO("Task 3 - No flag is set");
        }
        else
        {
            EZINFO("Task 3 - bit_mask = %d", bit_mask);
            if((bit_mask & FLAG0) == FLAG0)
            {
                EZINFO("Task 3 - Flag 0 is set");
                ezOsal_EventClear(&event, FLAG0);
            }

            if((bit_mask & FLAG1) == FLAG1)
            {
                EZINFO("Task 3 - Flag 1 is set");
                ezOsal_EventClear(&event, FLAG1);
            }

            if((bit_mask & FLAG2) == FLAG2)
            {
                EZINFO("Task 3 - Flag 2 is set. Stop Test application");
                ezOsal_EventClear(&event, FLAG2);

                ezOsal_TimerDelete(&timer);
                ezOsal_TaskDelete(&task3);
            }
        }
        ezOsal_TaskDelay(10);
    }
}

/* End of file*/
