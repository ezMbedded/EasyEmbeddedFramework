/*****************************************************************************
* Filename:         ez_osal.c
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

/** @file   ez_osal.c
 *  @author Hai Nguyen
 *  @date   15.03.2025
 *  @brief  Common interface for OS abstraction layer
 *
 *  @details None
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_osal.h"

#if (EZ_OSAL == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_OSAL_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_osal"       /**< module name */
#include "ez_logging.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define IS_INTERFACE_IMPLEMENTED(interface_p, function_p) ((interface_p != NULL) && (interface_p->function_p != NULL))

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static const ezOsal_Interfaces_t *osal_interface = NULL;

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/

/*****************************************************************************
* Function: ezOsal_Init
*//**
* @brief Initialize OS abstraction layer
*
* @details None
*
* @param[in]    argument: pointer to the argument
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_Init(NULL);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_Init(void *argument)
{
    EZTRACE("ezOsal_Init()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, Init))
    {
        return osal_interface->Init(argument);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Function: ezOsal_SetInterface
*//** 
* @brief Set the implemented OS interfaces
*
* @details Must be called first before calling any other OS functions
*
* @param[in]    interface: pointer to the implemented OS interfaces
* @return       ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_SetInterface(&implemented_interface);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_SetInterface(const ezOsal_Interfaces_t *interface)
{
    EZTRACE("ezOsal_SetInterface()");
    if(interface != NULL)
    {
        osal_interface = interface;
        return ezSUCCESS;
    }
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TaskCreate
*//** 
* @brief Create OS task
*
* @details None
*
* @param[in]    handle: handle storing task data
* @return       ezOsal_TaskHandle_t or NULL if failed
*
* @pre OS interface must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_TaskHandle_t handle;
* (void) ezOsal_TaskCreate(&handle);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_TaskCreate(ezOsal_TaskHandle_t* handle)
{
    EZTRACE("ezOsal_TaskCreate()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskCreate))
    {
        return osal_interface->TaskCreate(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TaskDelete
*//** 
* @brief Delete OS task
*
* @details None
*
* @param[in]    task_handle: task handle
* @return       ezSUCCESS or ezFAIL
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_TaskHandle_t task = ezOsal_TaskCreate("task1", 1024, 1, task_function, NULL);
* ezSTATUS status = ezOsal_TaskDelete(task);
* @endcode
*
* @see ezOsal_TaskCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TaskDelete(ezOsal_TaskHandle_t* handle)
{
    EZTRACE("ezOsal_TaskDelete()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskDelete))
    {
        return osal_interface->TaskDelete(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TaskSuspend
*//**
* @brief Suspend OS task
*
* @details None
*
* @param[in]    task_handle: task handle
* @return       ezSUCCESS or ezFAIL
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_TaskHandle_t task = ezOsal_TaskCreate("task1", 1024, 1, task_function, NULL);
* ezSTATUS status = ezOsal_TaskSuspend(task);
* @endcode
*
* @see ezOsal_TaskCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TaskSuspend(ezOsal_TaskHandle_t* handle)
{
    EZTRACE("ezOsal_TaskSuspend()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskSuspend))
    {
        return osal_interface->TaskSuspend(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TaskResume
*//** 
* @brief Resume OS task
*
* @details None
*
* @param[in]    task_handle: task handle
* @return       ezSUCCESS or ezFAIL
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_TaskHandle_t task = ezOsal_TaskCreate("task1", 1024, 1, task_function, NULL);
* ezSTATUS status = ezOsal_TaskResume(task);
* @endcode
*
* @see ezOsal_TaskCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TaskResume(ezOsal_TaskHandle_t* handle)
{
    EZTRACE("ezOsal_TaskResume()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskResume))
    {
        return osal_interface->TaskResume(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Function: ezOsal_TaskDelay
*//** 
* @brief    Delay OS task for cerntain number of ticks.
*
* @details  Relative delay in ticks, user must implement their own DelayUntil()
*           function
*
* @param[in]    num_of_ticks: number of delay ticks
* @return       ezSUCCESS or ezFAIL
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* @endcode
*
* @see ezOsal_TaskCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TaskDelay(unsigned long num_of_ticks)
{
    EZTRACE("ezOsal_TaskDelay()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskDelay))
    {
        return osal_interface->TaskDelay(num_of_ticks);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TaskGetTickCount
*//** 
* @brief    Get current tick counts
*
* @details  None
*
* @return   Number of ticks
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* @endcode
*
* @see ezOsal_TaskCreate
*
*****************************************************************************/
unsigned long ezOsal_TaskGetTickCount(void)
{
    EZTRACE("ezOsal_TaskGetTickCount()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskGetTickCount))
    {
        return osal_interface->TaskGetTickCount();
    }
    EZWARNING("Interface is not implemented");
    return 0;
}

/*****************************************************************************
* Function: ezOsal_TaskStartScheduler
*//**
* @brief Start OS task scheduler
*
* @details None
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_TaskStartScheduler();
* @endcode
*
*****************************************************************************/
void ezOsal_TaskStartScheduler(void)
{
    EZTRACE("ezOsal_TaskStartScheduler()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TaskStartScheduler))
    {
        osal_interface->TaskStartScheduler();
    }
    EZWARNING("Interface is not implemented");
}

/*****************************************************************************
* Function: ezOsal_SemaphoreCreate
*//** 
* @brief Create a semaphore
*
* @details None
*
* @param[in]    handle: semaphore handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_SemaphoreHandle_t sem;
* ezOsal_SemaphoreCreate(&sem);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_SemaphoreCreate(ezOsal_SemaphoreHandle_t *handle)
{
    EZTRACE("ezOsal_SemaphoreCreate()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, SemaphoreCreate))
    {
        return osal_interface->SemaphoreCreate(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_SemaphoreDelete
*//** 
* @brief Delete a semaphore
*
* @details None
*
* @param[in]    semaphore_handle: (IN)semaphore handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_SemaphoreHandle_t sem = ezOsal_SemaphoreCreate(2);
* ezSTATUS status = ezOsal_SemaphoreDelete(sem);
* @endcode
*
* @see ezOsal_SemaphoreCreate
*
*****************************************************************************/
ezSTATUS ezOsal_SemaphoreDelete(ezOsal_SemaphoreHandle_t *handle)
{
    EZTRACE("ezOsal_SemaphoreDelete()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, SemaphoreDelete))
    {
        return osal_interface->SemaphoreDelete(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_SemaphoreTake
*//** 
* @brief Take semaphore
*
* @details None
*
* @param[in]    semaphore_handle: semaphore handle
* @param[in]    timeout_ms: time to wait for the semaphore in millisecond
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_SemaphoreHandle_t sem = ezOsal_SemaphoreCreate(2);
* ezSTATUS status = ezOsal_SemaezOsal_SemaphoreTakephoreDelete(sem, 100);
* @endcode
*
* @see ezOsal_SemaphoreCreate
*
*****************************************************************************/
ezSTATUS ezOsal_SemaphoreTake(ezOsal_SemaphoreHandle_t *handle, uint32_t timeout_ticks)
{
    EZTRACE("ezOsal_SemaphoreTake(timeout_ticks = %d)", timeout_ticks);
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, SemaphoreTake))
    {
        return osal_interface->SemaphoreTake(handle, timeout_ticks);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_SemaphoreGive
*//** 
* @brief Give semaphore
*
* @details None
*
* @param[in]    semaphore_handle: semaphore handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_SemaphoreHandle_t sem = ezOsal_SemaphoreCreate(2);
* ezSTATUS status = ezOsal_SemaphoreGive(sem);
* @endcode
*
* @see ezOsal_SemaphoreCreate
*
*****************************************************************************/
ezSTATUS ezOsal_SemaphoreGive(ezOsal_SemaphoreHandle_t *handle)
{
    EZTRACE("ezOsal_SemaphoreGive()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, SemaphoreGive))
    {
        return osal_interface->SemaphoreGive(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TimerCreate
*//** 
* @brief Create a timer
*
* @details None
*
* @param[in]    handle: timer handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_TimerCreate(&handle);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_TimerCreate(ezOsal_TimerHandle_t *handle)
{
    EZTRACE("ezOsal_TimerCreate()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TimerCreate))
    {
        return osal_interface->TimerCreate(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TimerDelete
*//** 
* @brief Delete a timer
*
* @details None
*
* @param    timer_handle: timer handle
* @return   ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_TimerDelete(&timer);
* @endcode
*
* @see ezOsal_TimerCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TimerDelete(ezOsal_TimerHandle_t *handle)
{
    EZTRACE("ezOsal_TimerDelete()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TimerDelete))
    {
        return osal_interface->TimerDelete(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TimerStart
*//** 
* @brief Start a timer
*
* @details None
*
* @param    timer_handle: timer handle
* @return   ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_TimerStart(&timer);
* @endcode
*
* @see ezOsal_TimerCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TimerStart(ezOsal_TimerHandle_t *handle)
{
    EZTRACE("ezOsal_TimerStart()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TimerStart))
    {
        return osal_interface->TimerStart(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_TimerStop
*//** 
* @brief Stop a timer
*
* @details None
*
* @param    timer_handle: timer handle
* @return   ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezSTATUS status = ezOsal_TimerStop(&timer);
* @endcode
*
* @see ezOsal_TimerCreate
*
*****************************************************************************/
ezSTATUS ezOsal_TimerStop(ezOsal_TimerHandle_t *handle)
{
    EZTRACE("ezOsal_TimerStop()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, TimerStop))
    {
        return osal_interface->TimerStop(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}


/*****************************************************************************
* Function: ezOsal_EventCreate
*//**
* @brief Create a new event group
*
* @details None
*
* @param[in]    handle: event handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_EventHandle_t event;
* ezOsal_EventCreate(&event);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_EventCreate(ezOsal_EventHandle_t *handle)
{
    EZTRACE("ezOsal_EventCreate()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, EventCreate))
    {
        return osal_interface->EventCreate(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Function: ezOsal_EventDelete
*//**
* @brief Delete an event group
*
* @details None
*
* @param[in]    handle: event handle
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_EventHandle_t event;
* ezOsal_EventCreate(&event);
* ezOsal_EventDelete(&event);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_EventDelete(ezOsal_EventHandle_t *handle)
{
    EZTRACE("ezOsal_EventDelete()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, EventDelete))
    {
        return osal_interface->EventDelete(handle);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Function: ezOsal_EventWait
*//**
* @brief Wait for events
*
* @details None
*
* @param[in]    handle: event handle
* @param[in]    event_mask: event mask
* @param[in]    timeout_ticks: timeout in ticks
* @return       event mask
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_EventHandle_t event;
* ezOsal_EventCreate(&event);
* ezOsal_EventWait(&event, 0x01, 100);
* @endcode
*
*****************************************************************************/
int ezOsal_EventWait(ezOsal_EventHandle_t *handle, uint32_t event_mask, uint32_t timeout_ticks)
{
    EZTRACE("ezOsal_EventWait()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, EventWait))
    {
        return osal_interface->EventWait(handle, event_mask, timeout_ticks);
    }
    EZWARNING("Interface is not implemented");
    return 0;
}


/*****************************************************************************
* Function: ezOsal_EventSet
*//**
* @brief Set event
*
* @details None
*
* @param[in]    handle: event handle
* @param[in]    event_mask: event mask
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_EventHandle_t event;
* ezOsal_EventCreate(&event);
* ezOsal_EventSet(&event, 0x01);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_EventSet(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    EZTRACE("ezOsal_EventSet()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, EventSet))
    {
        return osal_interface->EventSet(handle, event_mask);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Function: ezOsal_EventClear
*//**
* @brief Clear events
*
* @details None
*
* @param[in]    handle: event handle
* @param[in]    event_mask: event mask
* @return       ezSTATUS
*
* @pre OS interfaces must be implemented
* @post None
*
* \b Example
* @code
* ezOsal_EventHandle_t event;
* ezOsal_EventCreate(&event);
* ezOsal_EventClear(&event, 0x01);
* @endcode
*
*****************************************************************************/
ezSTATUS ezOsal_EventClear(ezOsal_EventHandle_t *handle, uint32_t event_mask)
{
    EZTRACE("ezOsal_EventClear()");
    if(IS_INTERFACE_IMPLEMENTED(osal_interface, EventClear))
    {
        return osal_interface->EventClear(handle, event_mask);
    }
    EZWARNING("Interface is not implemented");
    return ezFAIL;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

#endif /* EZ_OSAL == 1 */
/* End of file*/
