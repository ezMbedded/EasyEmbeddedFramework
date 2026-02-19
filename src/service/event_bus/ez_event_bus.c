/*****************************************************************************
* Filename:         ez_event_notifier.c
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

/** @file   ez_event_notifier.c
 *  @author Hai Nguyen
 *  @date   27.02.2024
 *  @brief  Implementation of the event notifier component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_event_bus.h"

#if(EZ_EVENT_BUS == 1U)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_EVENT_LOGGING_LEVEL      /**< logging level */
#define MOD_NAME    "EVENT_NOTIFY"  /**< module name */

#include "string.h"
#include "ez_logging.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/



/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Public functions
*****************************************************************************/

ezSTATUS ezEventBus_CreateBus(
    ezEventBus_t *event_bus,
    uint8_t *buff,
    uint32_t buff_size)
{
    EZDEBUG("evntNoti_CreateSubject()");
    ezSTATUS status = ezFAIL;

    if(buff == NULL || buff_size == 0U || event_bus == NULL)
    {
        EZERROR("  Cannot create subject, invalid buffer");
        return ezSTATUS_ARG_INVALID;
    }

    ezLinkedList_InitNode(&event_bus->node);
    return ezQueue_CreateQueue(&event_bus->event_queue, buff, buff_size);
}


void ezEventBus_ResetBus(ezEventBus_t * event_bus)
{
    if (event_bus)
    {
        ezLinkedList_InitNode(&event_bus->node);
        uint32_t num_of_event = ezQueue_GetNumOfElement(&event_bus->event_queue);
        for (uint32_t i = 0; i < num_of_event; i++)
        {
            ezQueue_PopFront(&event_bus->event_queue);
        }

        num_of_event = ezQueue_GetNumOfElement(&event_bus->event_queue);
        if(num_of_event != 0U)
        {
            EZDEBUG("Event bus reset NOK");
        }
    }
}


ezSTATUS ezEventBus_CreateListener(ezEventListener_t *listener, EVENT_CALLBACK callback)
{
    EZDEBUG("evntNoti_CreateListener()");
    ezSTATUS status = ezFAIL;

    if (listener != NULL && callback != NULL)
    {
        listener->callback = callback;
        status = ezSUCCESS;
        EZDEBUG("  Create Observer OK");
    }
    else
    {
        EZERROR("  Cannot create observer, invalid argument");
    }

    return status;
}


ezSTATUS ezEventBus_Run(ezEventBus_t * event_bus)
{
    uint32_t event_code = 0U;
    void *data = NULL;
    uint32_t data_size = 0U;
    struct Node* it_node = NULL;
    ezEventListener_t *listener = NULL;
    
    if(event_bus == NULL)
    {
        EZWARNING("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    if(ezQueue_GetNumOfElement(&event_bus->event_queue) >= 2U)
    {
        /* Get event code */
        if(ezQueue_GetFront(
            &event_bus->event_queue,
            &data,
            &data_size) != ezSUCCESS)
        {
            EZERROR("Cannot get event code from queue");
            (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event code */
            (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event data */
            return ezFAIL;
        }

        
        if(data_size != sizeof(uint32_t))
        {
            EZERROR("Invalid event code size");
            (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event code */
            (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event data */
            return ezFAIL;
        }

        event_code = *(uint32_t*)data;
        (void)ezQueue_PopFront(&event_bus->event_queue);

        /* Get event data */
        if(ezQueue_GetFront(
            &event_bus->event_queue,
            (void*)&data,
            &data_size) != ezSUCCESS)
        {
            EZERROR("Cannot get event data from queue");
            (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event data */
            return ezFAIL;
        }

        /* Notify all listeners */
        EZ_LINKEDLIST_FOR_EACH(it_node, &event_bus->node)
        {
            listener = EZ_LINKEDLIST_GET_PARENT_OF(
                it_node,
                node,
                ezEventListener_t);
            if(listener != NULL && listener->callback != NULL)
            {
                listener->callback(event_code, data, data_size);
            }
        }

         (void)ezQueue_PopFront(&event_bus->event_queue); /* pop event data */
    }
    return ezSUCCESS;
}

ezSTATUS ezEventBus_Listen(ezEventBus_t *event_bus, ezEventListener_t *listener)
{
    EZDEBUG("evntNoti_SubscribeEvent()");
    ezSTATUS status = ezFAIL;

    if (event_bus != NULL
        && listener != NULL)
    {
        EZ_LINKEDLIST_ADD_HEAD(&event_bus->node, &listener->node);

        EZDEBUG("  subscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(&event_bus->node));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot subscribe - null pointer, or max subscribers reached");
    }

    return status;
}


ezSTATUS ezEventBus_Unlisten(ezEventBus_t *event_bus, ezEventListener_t *listener)
{
    EZDEBUG("evntNoti_UnsubscribeEvent()");
    ezSTATUS status = ezFAIL;

    if (event_bus != NULL &&
        listener != NULL &&
        ezLinkedList_IsNodeInList(&event_bus->node, &listener->node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(&listener->node);

        EZDEBUG("  unsubscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(&event_bus->node));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot unsubscribe - null pointer");
    }

    return status;
}


uint16_t ezEventBus_GetNumOfListeners(ezEventBus_t *event_bus)
{
    uint16_t num_of_listeners = 0;

    EZDEBUG("ezEventBus_GetNumOfListeners()");
    if (event_bus)
    {
        num_of_listeners = ezLinkedList_GetListSize(&event_bus->node);
        EZDEBUG("  num of listener = %d", num_of_listeners);
    }
    else
    {
        EZWARNING("  Invalid argument");
    }
    return num_of_listeners;
}


bool ezEventBus_SendEvent(
    ezEventBus_t *event_bus,
    uint32_t event_code,
    void *event_data,
    size_t event_data_size)
{
    ezReservedElement q_element_event_code = NULL;
    ezReservedElement q_element_data = NULL;
    void *code = NULL;
    void *data = NULL;

    EZDEBUG("evntNoti_NotifyEnvent()");
    if(event_bus == NULL)
    {
        EZWARNING("  Invalid argument");
        return false;
    }

    
    q_element_event_code = ezQueue_ReserveElement(&event_bus->event_queue, &code, sizeof(uint32_t));
    q_element_data = ezQueue_ReserveElement(&event_bus->event_queue, &data, event_data_size);

    if(q_element_event_code == NULL || q_element_data == NULL)
    {
        EZWARNING("Cannot reserve event queue element");
        ezQueue_ReleaseReservedElement(&event_bus->event_queue, q_element_event_code);
        ezQueue_ReleaseReservedElement(&event_bus->event_queue, q_element_data);
        return false;
    }

    *(uint32_t*)code = event_code;
    memcpy(data, event_data, event_data_size);
   
    ezQueue_PushReservedElement(&event_bus->event_queue, q_element_event_code);
    ezQueue_PushReservedElement(&event_bus->event_queue, q_element_data);

    return true;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
/* None */

#endif /* (EZ_EVENT_BUS == 1U) */
/* End of file*/
