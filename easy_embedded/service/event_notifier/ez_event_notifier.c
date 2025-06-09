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
#include "ez_event_notifier.h"

#if(EZ_EVENT_NOTIFIER == 1U)
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

ezSTATUS ezEventNotifier_CreateSubject(ezSubject *subject)
{
    EZDEBUG("evntNoti_CreateSubject()");
    ezSTATUS status = ezFAIL;

    if (subject)
    {
        ezLinkedList_InitNode(subject);
        status = ezSUCCESS;
        EZDEBUG("  Create OK");
    }
    else
    {
        EZERROR("  Cannot create subject, invalid argument");
    }

    return status;
}


void ezEventNotifier_ResetSubject(ezSubject * subject)
{
    if (subject)
    {
        ezLinkedList_InitNode(subject);
    }
}


ezSTATUS ezEventNotifier_CreateObserver(ezObserver *observer,
                                        EVENT_CALLBACK callback)
{
    EZDEBUG("evntNoti_CreateObserver()");
    ezSTATUS status = ezFAIL;

    if (observer != NULL && callback != NULL)
    {
        observer->callback = callback;
        status = ezSUCCESS;
        EZDEBUG("  Create Observer OK");
    }
    else
    {
        EZERROR("  Cannot create observer, invalid argument");
    }

    return status;
}


ezSTATUS ezEventNotifier_SubscribeToSubject(ezSubject *subject,
                                            ezObserver *observer)
{
    EZDEBUG("evntNoti_SubscribeEvent()");
    ezSTATUS status = ezFAIL;

    if (subject != NULL
        && observer != NULL)
    {
        EZ_LINKEDLIST_ADD_HEAD(subject, &observer->node);

        EZDEBUG("  subscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(subject));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot subscribe - null pointer, or max subscribers reached");
    }

    return status;
}


ezSTATUS ezEventNotifier_UnsubscribeFromSubject(ezSubject *subject,
                                                ezObserver *observer)
{
    EZDEBUG("evntNoti_UnsubscribeEvent()");
    ezSTATUS status = ezFAIL;

    if (subject != NULL &&
        observer != NULL &&
        ezLinkedList_IsNodeInList(subject, &observer->node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(&observer->node);

        EZDEBUG("  unsubscribing success");
        EZDEBUG("  num of subscriber [num = %d]", ezLinkedList_GetListSize(subject));
        status = ezSUCCESS;
    }
    else
    {
        EZWARNING("  cannot unsubscribe - null pointer");
    }

    return status;
}


uint16_t ezEventNotifier_GetNumOfObservers(ezSubject *subject)
{
    uint16_t num_of_observers = 0;

    EZDEBUG("ezEventNotifier_GetNumOfObservers()");

    if (subject)
    {
        num_of_observers = ezLinkedList_GetListSize(subject);
        EZDEBUG("  num of observer = %d", num_of_observers);
    }
    else
    {
        EZWARNING("  Invalid argument");
    }
    return num_of_observers;
}


void ezEventNotifier_NotifyEvent(ezSubject *subject,
                                 uint32_t event_code,
                                 void *param1,
                                 void *param2)
{
    EZDEBUG("evntNoti_NotifyEnvent()");

    struct Node *iterate = NULL;
    struct ezObserver *sub = NULL;

    if (subject != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(iterate, subject)
        {
            sub = EZ_LINKEDLIST_GET_PARENT_OF(iterate, node, struct ezObserver);
            if (sub->callback)
            {
                EZDEBUG("  notify observer");
                sub->callback(event_code, param1, param2);
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
