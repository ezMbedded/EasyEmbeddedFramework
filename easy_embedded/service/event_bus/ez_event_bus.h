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
 *         param1: pointer to first parameter, can be NULL if not used
 *         param2: pointer to second parameter, can be NULL if not used
 *
 */
typedef int (*EVENT_CALLBACK)(uint32_t event_code, void *param1, void *param2);


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
typedef struct Node ezEventBus_t;


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
* instead of modify the struct by themselve.
*
* @param[in]    *event_bus: Pointer to the event bus
* @return       ezSTATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* ezEventBus_t subject;
* (void) ezEventNotifier_CreateSubject(&subject);
* @endcode
*
*****************************************************************************/
ezSTATUS ezEventBus_CreateBus(ezEventBus_t *event_bus);


/******************************************************************************
* Function: ezEventBus_ResetBus
*//**
* @brief This function resets an event bus
*
* @details It resets the linked list and unlinks all nodes of that list
*
* @param[in]    *event_bus: Pointer to the event bus
* @return       None
*
* @pre Subject must be created
* @post None
*
* \b Example
* @code
* ezEventNotifier_ResetSubject(&subject);
* @endcode
*
* @see ezEventNotifier_CreateSubject
*
*******************************************************************************/
void ezEventBus_ResetBus(ezEventBus_t *event_bus);


/******************************************************************************
* Function: ezEventNotifier_CreateListener
*//**
* @brief This function creates an observer for the events from a subject.
*
* @details The user is RECOMMENDED to use this function to create the observer
* instead of modify the struct by themselve.
*
* @param[in]    *observer: Pointer to the obsever
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
* @pre subject and observer must be created
* @post None
*
* \b Example
* @code
* ezEventNotifier_SubscribeEvent(&subject, &observer);
* @endcode
*
* @see ezEventNotifier_CreateSubject, ezEventNotifier_CreateObserver
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
* @param[in]    *event_bus: Pointer to the event bus
* @param[in]    *observer: Publisher handle
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
* @param[in]   *event_bus: event bus
* @return   number of observers
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
* @brief This function send an event to the bus
*
* @details
*
* @param[in]    *event_bus: Pointer to the event bus
* @param[in]    event_code: Event code. Defined by users
* @param[in]    param1: Companion paramneter 1, set to NULL if unused
* @param[in]    param1: Companion paramneter 2, set to NULL if unused
* @return       None
*
* @pre  subject must be created. event_code, param1, and param2 are defined by
*       the user
* @post None
*
* \b Example
* @code
* evntNoti_NotifyEnvent(&subject, ENUM_ERROR_CODE, NULL, NULL);
* @endcode
*
* @see ezEventNotifier_CreateSubject
*
*******************************************************************************/
void ezEventBus_SendEvent(
    ezEventBus_t *event_bus,
    uint32_t event_code,
    void *param1,
    void *param2);

#endif /* (EZ_EVENT_BUS == 1U) */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_EVENT_BUS_H */

/* End of file */
