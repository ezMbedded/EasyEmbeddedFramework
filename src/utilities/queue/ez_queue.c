/*****************************************************************************
* Filename:         ez_queue.c
* Author:           Hai Nguyen
* Original Date:    26.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_queue.c
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Implementation of the queue data structure
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_queue.h"

#if (EZ_QUEUE == 1U)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_QUEUE_LOGGING_LEVEL       /**< logging level */
#define MOD_NAME    "ez_queue"     /**< module name */

#include "ez_logging.h"
#include <string.h>

/*****************************************************************************
* Module Preprocessor Macros
*****************************************************************************/
#define ITEM_HDR_SIZE  EZ_ALIGN_UP(sizeof(ezQueueItem), CONFIG_STATIC_ALLOC_ALIGNMENT)
#define ITEM_DATA(it)  ((void*)((uint8_t*)(it) + ITEM_HDR_SIZE))

/*****************************************************************************
* Module Typedefs
*****************************************************************************/


/** @brief item of the queue structure
 *
 */
typedef struct
{
    size_t item_size; /**< header size + data size */
    size_t data_size; /**< data size */
    struct Node node; /**< node of the linked list */
}ezQueueItem;

/*****************************************************************************
* Module Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* External functions
*****************************************************************************/


ezSTATUS ezQueue_CreateQueue(ezQueue *queue, uint8_t *buff, uint32_t buff_size)
{
    ezSTATUS status = ezFAIL;

    EZTRACE("ezQueue_CreateQueue( size = %u)", (unsigned int)buff_size);

    if (queue != NULL && buff != NULL && buff_size > 0)
    {
        ezLinkedList_InitNode(&queue->q_item_list);
        /* clear before the init: the allocator stores its records in the buffer */
        memset(buff, 0, buff_size);
        if (ezStaticAlloc_InitMemList(&queue->mem_list, buff, buff_size) == true)
        {
            queue->num_of_element = 0;
            status = ezSUCCESS;
            EZDEBUG("create queue success");
        }
    }

    return status;
}

ezSTATUS ezQueue_PopFront(ezQueue* queue)
{
    ezSTATUS status = ezFAIL;
    ezQueueItem* popped_item = NULL;

    EZTRACE("ezQueue_PopFront()");

    if (queue != NULL)
    {
        if (queue->num_of_element > 0)
        {
            popped_item = EZ_LINKEDLIST_GET_PARENT_OF(queue->q_item_list.next, node, ezQueueItem);
            /* the item leaves the queue even if the free below fails */
            EZ_LINKEDLIST_UNLINK_NODE(&popped_item->node);
            queue->num_of_element--;
            if (ezStaticAlloc_Free(&queue->mem_list, (void*)popped_item) == true)
            {
                status = ezSUCCESS;
            }
            else
            {
                EZTRACE("free failed");
            }
        }
        else
        {
            EZDEBUG("queue is empty");
        }
    }

    return status;
}


ezSTATUS ezQueue_PopBack(ezQueue *queue)
{
    ezSTATUS status = ezFAIL;
    ezQueueItem *popped_item = NULL;

    EZTRACE("ezQueue_PopBack()");

    if (queue != NULL)
    {
        if (queue->num_of_element > 0)
        {
            popped_item = EZ_LINKEDLIST_GET_PARENT_OF(queue->q_item_list.prev, node, ezQueueItem);
            /* the item leaves the queue even if the free below fails */
            EZ_LINKEDLIST_UNLINK_NODE(&popped_item->node);
            queue->num_of_element--;
            if (ezStaticAlloc_Free(&queue->mem_list, (void*)popped_item) == true)
            {
                status = ezSUCCESS;
            }
            else
            {
                EZTRACE("free failed");
            }
        }
        else
        {
            EZDEBUG("queue is empty");
        }
    }

    return status;
}


ezReservedElement ezQueue_ReserveElement(ezQueue *queue, void **data, uint32_t data_size)
{
    ezQueueItem* item = NULL;
    size_t alloc_size = 0;

    if (queue != NULL && data != NULL && data_size > 0 && data_size <= (SIZE_MAX - ITEM_HDR_SIZE))
    {
        EZTRACE("ezQueue_ReserveElement( [size = %u])", (unsigned int)data_size);
        alloc_size = ITEM_HDR_SIZE + data_size;
        item = (ezQueueItem*)ezStaticAlloc_Malloc(&queue->mem_list, alloc_size);

        if (item != NULL)
        {
            item->item_size = alloc_size;
            item->data_size = data_size;
            ezLinkedList_InitNode(&item->node);
            *data = ITEM_DATA(item);
        }
    }

    return (ezReservedElement)item;
}


ezSTATUS ezQueue_PushReservedElement(ezQueue *queue, ezReservedElement element)
{
    ezSTATUS status = ezFAIL;
    ezQueueItem *item = (ezQueueItem *)element;

    if (queue != NULL && element != NULL)
    {
        if (item->node.next != &item->node)
        {
            EZDEBUG("element is already in the queue");
        }
        else if (EZ_LINKEDLIST_ADD_TAIL(&queue->q_item_list, &item->node) == true)
        {
            queue->num_of_element++;
            status = ezSUCCESS;
        }
    }

    return status;
}


ezSTATUS ezQueue_ReleaseReservedElement( ezQueue *queue, ezReservedElement element)
{
    ezSTATUS status = ezFAIL;
    ezQueueItem *item = element;

    if (queue != NULL && item != NULL)
    {
        if (item->node.next == &item->node)
        {
            if (ezStaticAlloc_Free(&queue->mem_list, (void *)item) == true)
            {
                status = ezSUCCESS;
            }
        }
    }
    return status;
}


ezSTATUS ezQueue_Push(ezQueue* queue, void *data, uint32_t data_size)
{
    ezSTATUS status = ezSUCCESS;
    void *reserve_data = NULL;
    ezReservedElement reserved_elem = NULL;

    EZTRACE("ezQueue_Push( [@ = %p], [size = %u])", data, (unsigned int)data_size);

    if (queue != NULL && data != NULL && data_size > 0)
    {
        reserved_elem = ezQueue_ReserveElement(queue, &reserve_data, data_size);

        if (reserved_elem != NULL)
        {
            memcpy(reserve_data, data, data_size);
            status = ezQueue_PushReservedElement(queue, reserved_elem);
            if (status != ezSUCCESS)
            {
                (void)ezQueue_ReleaseReservedElement(queue, reserved_elem);
            }
        }
        else
        {
            status = ezFAIL;
            EZDEBUG("add item fail");
        }
    }
    else
    {
        status = ezFAIL;
    }

    return status;
}

ezSTATUS ezQueue_GetFront(ezQueue* queue, void **data, size_t *data_size)
{
    ezSTATUS status = ezSUCCESS;
    ezQueueItem* front_item = NULL;

    EZTRACE("ezQueue_GetFront()");

    if (queue != NULL && data != NULL && data_size != NULL)
    {
        if (queue->num_of_element > 0)
        {
            front_item = EZ_LINKEDLIST_GET_PARENT_OF(queue->q_item_list.next, node, ezQueueItem);
            *data = ITEM_DATA(front_item);
            *data_size = front_item->data_size;

#if (DEBUG_LVL == LVL_TRACE)
            EZTRACE("[item address = %p]", (void*)front_item);
            EZTRACE("[item node address = %p]", (void*)&front_item->node);
            EZTRACE("[item data size = %lu]", (unsigned long)front_item->data_size);

            EZTRACE("data of front item");
            EZHEXDUMP((uint8_t*)*data, (uint16_t)*data_size);
#endif /* DEBUG_LVL == LVL_TRACE */
        }
        else
        {
            EZDEBUG("queue is empty");
            status = ezFAIL;
        }
    }
    else
    {
        EZDEBUG("get front item fail");
        status = ezFAIL;
    }

    return status;
}

ezSTATUS ezQueue_GetBack(ezQueue* queue, void **data, size_t *data_size)
{
    ezSTATUS status = ezSUCCESS;
    ezQueueItem *back_item = NULL;

    EZTRACE("ezQueue_GetBack()");

    if (queue != NULL && data != NULL && data_size != NULL)
    {
        if (queue->num_of_element > 0)
        {
            back_item = EZ_LINKEDLIST_GET_PARENT_OF(queue->q_item_list.prev, node, ezQueueItem);
            *data = ITEM_DATA(back_item);
            *data_size = back_item->data_size;

#if (DEBUG_LVL == LVL_TRACE)
            EZTRACE("data of back item");
            EZHEXDUMP((uint8_t*)*data, (uint16_t)*data_size);
#endif
        }
        else
        {
            EZDEBUG("queue is empty");
            status = ezFAIL;
        }
    }
    else
    {
        EZDEBUG("get back item fail");
        status = ezFAIL;
    }

    return status;
}

uint32_t ezQueue_GetNumOfElement(ezQueue* queue)
{
    uint32_t num_of_element = 0;

    if (queue != NULL)
    {
        num_of_element = queue->num_of_element;
    }

    return num_of_element;
}


bool ezQueue_IsQueueReady(ezQueue *queue)
{
    bool is_ready = false;

    if (queue != NULL && ezStaticAlloc_IsMemListReady(&queue->mem_list) == true)
    {
        is_ready = true;
    }

    return is_ready;
}


/*****************************************************************************
* Internal functions
*****************************************************************************/
/* None */


#endif /* CONFIG_EZ_QUEUE == 1U */
/* End of file*/

