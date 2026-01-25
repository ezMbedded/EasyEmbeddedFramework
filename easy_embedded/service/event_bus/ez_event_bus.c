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

#include "ez_logging.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */


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

ezSTATUS ezEventBus_CreateBus(ezEventBus_t *event_bus)
{
    EZDEBUG("evntNoti_CreateSubject()");
    ezSTATUS status = ezFAIL;

    if (event_bus)
    {
        ezLinkedList_InitNode(event_bus);
        status = ezSUCCESS;
        EZDEBUG("  Create OK");
    }
    else
    {
        EZERROR("  Cannot create subject, invalid argument");
    }

    return status;
}


void ezEventBus_ResetBus(ezEventBus_t * event_bus)
{
    if (event_bus)
    {
        ezLinkedList_InitNode(event_bus);
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


ezSTATUS ezEventBus_Listen(ezEventBus_t *event_bus, ezEventListener_t *listener)
{
    EZDEBUG("evntNoti_SubscribeEvent()");
    ezSTATUS status = ezFAIL;

    if (event_bus != NULL
        && listener != NULL)
    {
        EZ_LINKEDLIST_ADD_HEAD(event_bus, &listener->node);

        EZDEBUG("  subscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(event_bus));
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
        ezLinkedList_IsNodeInList(event_bus, &listener->node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(&listener->node);

        EZDEBUG("  unsubscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(event_bus));
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
    uint16_t num_of_observers = 0;

    EZDEBUG("ezEventBus_GetNumOfListeners()");
    if (event_bus)
    {
        num_of_observers = ezLinkedList_GetListSize(event_bus);
        EZDEBUG("  num of observer = %d", num_of_observers);
    }
    else
    {
        EZWARNING("  Invalid argument");
    }
    return num_of_observers;
}


void ezEventBus_SendEvent(
    ezEventBus_t *event_bus,
    uint32_t event_code,
    void *param1,
    void *param2)
{
    EZDEBUG("evntNoti_NotifyEnvent()");

    struct Node *iterate = NULL;
    ezEventListener_t *listener = NULL;

    if (event_bus != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(iterate, event_bus)
        {
            listener = EZ_LINKEDLIST_GET_PARENT_OF(iterate, node, ezEventListener_t);
            if (listener->callback)
            {
                EZDEBUG("  notify observer");
                listener->callback(event_code, param1, param2);
            }
        }
    }
    else
    {
        EZWARNING("  Invalid argument");
    }
}

/*****************************************************************************
* Local functions
*****************************************************************************/
/* None */

#endif /* (EZ_EVENT_NOTIFIER == 1U) */
/* End of file*/
