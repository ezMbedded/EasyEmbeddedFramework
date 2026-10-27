/*****************************************************************************
* Filename:         ez_event_bus.h
* Author:           Hai Nguyen
* Original Date:    27.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_event_bus.h
 *  @author Hai Nguyen
 *  @date   27.02.2024
 *  @brief  Public API of the event notifier component.
 *
 *  @details Event notifier component implements the observer design pattern.
 */

#ifndef _EZ_EVENT_BUS_H
#define _EZ_EVENT_BUS_H

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if(EZ_EVENT_BUS == 1U)
#include <stdint.h>
#include <stdbool.h>
#include "ez_linked_list.h"
#include "ez_queue.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief event callback pointer. It is used by the event notifier module to 
 *         notify the subscriber about an event.
 *         event_code: number representing an event. Specified by the event
 *         subject
 *         data: pointer to the event data. NULL if the event has no data. The
 *         data is only valid during the callback, copy it to keep it
 *         data_size: size of the event data. 0 if the event has no data
 *
 *  @warning The callback is called from ezEventBus_Run() while the bus is
 *           delivering an event. It MUST NOT call ezEventBus_Unlisten() or
 *           ezEventBus_ResetBus() on that bus, otherwise the delivery walks a
 *           list that is being modified and the event data is released while
 *           the other listeners still use it. Calling ezEventBus_SendEvent()
 *           is allowed, the new event is delivered by the next run.
 */
typedef int (*EVENT_CALLBACK)(uint32_t event_code, const void *data, size_t data_size);


/** @brief Observer object, used to subscribed to a subject to receive event
 *         notification
 */
struct ezEventListener
{
    struct Node node;           /**< linked list node */
    EVENT_CALLBACK callback;    /**< event call back function */
};


/** @brief define event_subject type.
 */
typedef struct{
    struct Node node;                               /**< linked list node */
    ezQueue event_queue;                            /**< event queue */
} ezEventBus_t;


/** @brief define event_observer type.
 */
typedef struct ezEventListener ezEventListener_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function : ezEventBus_CreateBus
*//** 
* @brief This function creates an event bus
*
* @details The user is RECOMMENDED to use this function to create the subject
* instead of modify the struct by themselves.
*
* @param[in]    event_bus: Pointer to the event bus
* @param[in]    buff: Memory buffer providing to the event bus to work
* @param[in]    buff_size: Size of the memory buffer
* @return       ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezEventBus_t subject;
* uint8_t buffer[128];
* (void) ezEventBus_CreateBus(&subject, buffer, sizeof(buffer));
* @endcode
*
*****************************************************************************/
ezSTATUS ezEventBus_CreateBus(
    ezEventBus_t *event_bus,
    uint8_t *buff,
    uint32_t buff_size);


/******************************************************************************
* Function: ezEventBus_ResetBus
*//**
* @brief This function resets an event bus
*
* @details It unlinks all listeners from the bus, so they can listen again, and
*          drops all the events that are waiting in the queue.
*          It must not be called from a listener callback, see EVENT_CALLBACK.
*
* @param[in]    event_bus: Pointer to the event bus
* @return       None
*
* @pre Subject must be created
* @post None
*
* \b Example
* @code
* ezEventBus_ResetBus(&event_bus);
* @endcode
*
* @see ezEventBus_CreateBus
*
*******************************************************************************/
void ezEventBus_ResetBus(ezEventBus_t *event_bus);


/******************************************************************************
* Function: ezEventBus_CreateListener
*//**
* @brief This function creates an observer for the events from a subject.
*
* @details The user is RECOMMENDED to use this function to create the observer
* instead of modify the struct by themselves.
*
* @param[in]    listener: Pointer to the listener
* @param[in]    callback: Callback to handle event from the subject
* @return       ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezEventListener_t listener;
* (void) ezEventBus_CreateListener(&listener, EventHandleFunction);
* @endcode
*
*******************************************************************************/
ezSTATUS ezEventBus_CreateListener(ezEventListener_t * listener, EVENT_CALLBACK callback);


/******************************************************************************
 * Function: ezEventBus_Run
 *//**
 * @brief This function runs the event bus to process events and notify
 *       listeners.
 * 
 * @details This function should be called periodically to ensure that events
 *         are processed and listeners are notified in a timely manner.
 *         It delivers the events that are in the queue when it is called, in
 *         the order they were sent. Events sent by the listeners during the
 *         delivery are delivered by the next call.
 *         An event that cannot be read from the queue is dropped and the
 *         function returns ezFAIL after delivering the other events.
 * @param[in]    event_bus: Pointer to the event bus
 * @return       ezSTATUS
 *
 * @warning Listeners must not call ezEventBus_Unlisten() or
 *          ezEventBus_ResetBus() on the bus from their callback, see
 *          EVENT_CALLBACK.
 *
 * @pre Event bus must be created
 * @post None
 * \b Example
 * @code
 * ezEventBus_Run(&event_bus);
 * @endcode
 * 
 ******************************************************************************/
ezSTATUS ezEventBus_Run(ezEventBus_t * event_bus);


/******************************************************************************
* Function: ezEventBus_Listen
*//**
* @brief This function listen to an event bus
*
* @details
*
* @param[in]    *event_bus: Pointer to the event bus
* @param[in]    *listener: Publisher handle
* @return       ezSTATUS
*
* @pre subject and listener must be created
* @post None
*
* \b Example
* @code
* ezEventBus_Listen(&subject, &listener);
* @endcode
*
* @see ezEventBus_CreateBus, ezEventBus_CreateListener
*
*******************************************************************************/
ezSTATUS ezEventBus_Listen(ezEventBus_t *event_bus,
                           ezEventListener_t *listener);


/******************************************************************************
* Function: ezEventBus_Unlisten
*//**
* @brief This function unlisten from an event bus
*
* @details
*
* @param[in]    event_bus: Pointer to the event bus
* @param[in]    listener: Publisher handle
* @return       ezSTATUS
*
* @pre event_bus and listener must be created
* @post None
*
* \b Example
* @code
* ezEventNotifier_UnsubscribeEvent(&event_bus, &observer);
* @endcode
*
* @see ezEventNotifier_CreateSubject, ezEventNotifier_CreateObserver
*
*******************************************************************************/
ezSTATUS ezEventBus_Unlisten(ezEventBus_t *event_bus, ezEventListener_t *listener);


/******************************************************************************
* Function: ezEventBus_GetNumOfListeners
*//**
* @brief This function returns the number of observers in a subject
*
* @details
*
* @param[in]   event_bus: event bus
* @return   number of listeners
*
* @pre subject must be created
* @post None
*
* \b Example
* @code
* uint16_t num_observer = ezEventBus_GetNumOfListeners(&subject);
* @endcode
*
* @see ezEventNotifier_CreateSubject
*
*******************************************************************************/
uint16_t ezEventBus_GetNumOfListeners(ezEventBus_t *event_bus);


/******************************************************************************
* Function: ezEventBus_SendEvent
*//**
* @brief This function sends an event to the bus
*
* @details The event is copied to the queue of the bus and delivered to the
*          listeners by the next ezEventBus_Run(). It can be called from a
*          listener callback.
*          The event data is optional. If event_data is NULL or event_data_size
*          is 0, the event has no data: the listeners get NULL and 0.
*
* @param[in]    event_bus: Pointer to the event bus
* @param[in]    event_code: Event code. Defined by users
* @param[in]    event_data: Pointer to the event data, which is copied. NULL if
*                           the event has no data
* @param[in]    event_data_size: Size of the event data in bytes. 0 if the event
*                                has no data
* @return       ezSUCCESS if the event is queued,
*               ezSTATUS_ARG_INVALID if event_bus is NULL or the data is too big,
*               ezFAIL if the queue has no space for the event
*
* @pre  Event bus must be created. event_code and the data are defined by the
*       user
* @post None
*
* \b Example
* @code
* (void) ezEventBus_SendEvent(&event_bus, ENUM_ERROR_CODE, NULL, 0);
* @endcode
*
* @see ezEventBus_CreateBus, ezEventBus_Run
*
*******************************************************************************/
ezSTATUS ezEventBus_SendEvent(
    ezEventBus_t *event_bus,
    uint32_t event_code,
    const void *event_data,
    size_t event_data_size);


#endif /* (EZ_EVENT_BUS == 1U) */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_EVENT_BUS_H */

/* End of file */
