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
#define MOD_NAME    "EVENT_BUS"     /**< module name */

#include "string.h"
#include "ez_logging.h"
#include "ez_utilities_common.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/** @brief Size of the event header in the queue element, the event data follows
 *         it. Aligned like the queue does, so the data is aligned too
 */
#define EZ_EVENT_BUS_EVENT_HEADER_SIZE  EZ_ALIGN_UP(sizeof(ezEventBus_EventHeader_t), CONFIG_STATIC_ALLOC_ALIGNMENT)

/** @brief Get the event data, which is right after the header */
#define EZ_EVENT_BUS_EVENT_DATA(header) ((void *)((uint8_t *)(header) + EZ_EVENT_BUS_EVENT_HEADER_SIZE))


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Event header structure */
typedef struct
{
    uint32_t event_code;    /**< Event code */
    size_t event_data_size; /**< Size of event data, 0 if the event has no data */
}ezEventBus_EventHeader_t;


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
    EZDEBUG("ezEventBus_CreateBus()");

    if(buff == NULL || buff_size == 0U || event_bus == NULL)
    {
        EZERROR("  Cannot create event bus, invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    ezLinkedList_InitNode(&event_bus->node);
    return ezQueue_CreateQueue(&event_bus->event_queue, buff, buff_size);
}


void ezEventBus_ResetBus(ezEventBus_t * event_bus)
{
    if (event_bus)
    {
        /* Unlink every listener, so they are not left pointing into this bus */
        while(ezLinkedList_RemoveFirst(&event_bus->node) != NULL)
        {
            /* Nothing to do */
        }

        uint32_t num_of_event = ezQueue_GetNumOfElement(&event_bus->event_queue);
        while(num_of_event > 0U
              && ezQueue_PopFront(&event_bus->event_queue) == ezSUCCESS)
        {
            num_of_event--;
        }

        if(ezQueue_GetNumOfElement(&event_bus->event_queue) != 0U)
        {
            EZDEBUG("Event bus reset NOK");
        }
    }
}


ezSTATUS ezEventBus_CreateListener(ezEventListener_t *listener, EVENT_CALLBACK callback)
{
    EZDEBUG("ezEventBus_CreateListener()");
    ezSTATUS status = ezFAIL;

    if (listener != NULL && callback != NULL)
    {
        listener->callback = callback;
        status = ezSUCCESS;
        EZDEBUG("  Create listener OK");
    }
    else
    {
        EZERROR("  Cannot create listener, invalid argument");
    }

    return status;
}


ezSTATUS ezEventBus_Run(ezEventBus_t * event_bus)
{
    ezSTATUS status = ezSUCCESS;
    uint32_t num_of_events = 0U;
    size_t element_size = 0U;
    struct Node* it_node = NULL;
    ezEventListener_t *listener = NULL;
    ezEventBus_EventHeader_t *header = NULL;

    if(event_bus == NULL)
    {
        EZWARNING("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    num_of_events = ezQueue_GetNumOfElement(&event_bus->event_queue);
    while(num_of_events > 0U)
    {
        /* Get the event at the front of the queue */
        if(ezQueue_GetFront(
            &event_bus->event_queue,
            (void**)&header,
            &element_size) != ezSUCCESS)
        {
            /* Drop the event, or the queue would be stuck on it forever */
            EZERROR("Cannot get event from queue");
            (void)ezQueue_PopFront(&event_bus->event_queue);
            num_of_events--;
            status = ezFAIL;
            continue;
        }

        /* Notify all listeners */
        EZ_LINKEDLIST_FOR_EACH(it_node, &event_bus->node)
        {
            listener = EZ_LINKEDLIST_GET_PARENT_OF(
                it_node,
                node,
                ezEventListener_t);
            if(listener->callback != NULL)
            {
                listener->callback(
                    header->event_code,
                    (header->event_data_size > 0U) ? EZ_EVENT_BUS_EVENT_DATA(header) : NULL,
                    header->event_data_size);
            }
        }
        (void)ezQueue_PopFront(&event_bus->event_queue);
        num_of_events--;
    }
    return status;
}

ezSTATUS ezEventBus_Listen(ezEventBus_t *event_bus, ezEventListener_t *listener)
{
    EZDEBUG("ezEventBus_Listen()");
    ezSTATUS status = ezFAIL;

    if (event_bus != NULL
        && listener != NULL)
    {
        if(ezLinkedList_IsNodeInList(&event_bus->node, &listener->node))
        {
            EZWARNING("  listener is already listening to this bus");
            return ezFAIL;
        }

        EZ_LINKEDLIST_ADD_HEAD(&event_bus->node, &listener->node);

        EZDEBUG("  listen success");
        EZDEBUG("  num of listener [num = %d]", ezLinkedList_GetListSize(&event_bus->node));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot listen - invalid argument");
    }

    return status;
}


ezSTATUS ezEventBus_Unlisten(ezEventBus_t *event_bus, ezEventListener_t *listener)
{
    EZDEBUG("ezEventBus_Unlisten()");
    ezSTATUS status = ezFAIL;

    if (event_bus != NULL &&
        listener != NULL &&
        ezLinkedList_IsNodeInList(&event_bus->node, &listener->node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(&listener->node);

        EZDEBUG("  unlisten success");
        EZDEBUG("  num of listener [num = %d]", ezLinkedList_GetListSize(&event_bus->node));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot unlisten - invalid argument or listener not on this bus");
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


ezSTATUS ezEventBus_SendEvent(
    ezEventBus_t *event_bus,
    uint32_t event_code,
    const void *event_data,
    size_t event_data_size)
{
    ezReservedElement q_element = NULL;
    size_t alloc_size = 0U;
    ezEventBus_EventHeader_t *header = NULL;

    EZDEBUG("ezEventBus_SendEvent()");
    if(event_bus == NULL)
    {
        EZWARNING("  Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    /* An event without data is valid: no data or no size means an empty event */
    if(event_data == NULL || event_data_size == 0U)
    {
        event_data = NULL;
        event_data_size = 0U;
    }

    if(event_data_size > (UINT32_MAX - EZ_EVENT_BUS_EVENT_HEADER_SIZE - CONFIG_STATIC_ALLOC_ALIGNMENT))
    {
        EZWARNING("  Event data too big");
        return ezSTATUS_ARG_INVALID;
    }

    alloc_size = EZ_EVENT_BUS_EVENT_HEADER_SIZE + event_data_size;
    q_element = ezQueue_ReserveElement(&event_bus->event_queue, (void**)&header, (uint32_t)alloc_size);

    if(q_element == NULL)
    {
        EZWARNING("Cannot reserve event queue element");
        return ezFAIL;
    }

    header->event_code = event_code;
    header->event_data_size = event_data_size;
    if(event_data != NULL)
    {
        memcpy(EZ_EVENT_BUS_EVENT_DATA(header), event_data, event_data_size);
    }

    if(ezQueue_PushReservedElement(&event_bus->event_queue, q_element) != ezSUCCESS)
    {
        EZWARNING("Cannot push event queue element");
        (void)ezQueue_ReleaseReservedElement(&event_bus->event_queue, q_element);
        return ezFAIL;
    }

    return ezSUCCESS;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
/* None */

#endif /* (EZ_EVENT_BUS == 1U) */
/* End of file*/
