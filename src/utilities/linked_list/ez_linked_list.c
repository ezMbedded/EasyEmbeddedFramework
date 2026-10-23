/*****************************************************************************
* Filename:         ez_linked_list.c
* Author:           Hai Nguyen
* Original Date:    19.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_linked_list.c
 *  @author Hai Nguyen
 *  @date   19.02.2024
 *  @brief  Implementation of the linked list data structure
 *
 *  @details
 */


/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_linked_list.h"

#if (EZ_LINKEDLIST == 1U)

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
* External functions
*****************************************************************************/
void ezLinkedList_InitNode(struct Node* node)
{
    node->next = node;
    node->prev = node;
}


uint16_t ezLinkedList_GetListSize(struct Node* list_head)
{
    uint16_t size = 0;
    struct Node* it_node = NULL;

    if(list_head != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(it_node, list_head)
        {
            size++;
        }
    }
    return size;
}


bool ezLinkedList_AppendNode(struct Node* new_node, struct Node* node)
{
    bool is_success = false;
    if (new_node != NULL && node != NULL)
    {
        new_node->next = node->next;
        new_node->prev = node;
        node->next->prev = new_node;
        node->next = new_node;

        is_success = true;
    }

    return is_success;
}


struct Node * ezLinkedList_RemoveFirst(struct Node * list)
{
    struct Node* removed_node = NULL;

    if(list == NULL)
    {
        /* do nothing */
    }
    else if(list->next != list)
    {
        removed_node = list->next;
        EZ_LINKEDLIST_UNLINK_NODE(removed_node);
    }

    return removed_node;
}


bool ezLinkedList_IsNodeInList(struct Node* head, struct Node* searched_node)
{
    bool is_existing = false;
    struct Node* node;

    if (head != NULL && searched_node != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(node, head)
        {
            if (node == searched_node)
            {
                is_existing = true;
                break;
            }
        }
    }
    else
    {
        /* could not find node, return false */
    }

    return is_existing;
}


#endif /* (EZ_LINKEDLIST == 1U) */
/* End of file*/
