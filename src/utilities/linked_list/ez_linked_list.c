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

#define NODE_INVALID_ID     0xFFFF

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
//static struct Node node_pool[NUM_OF_NODE] = { 0 };

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
 
    EZ_LINKEDLIST_FOR_EACH(it_node, list_head)
    {
        size++;
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


struct Node* ezLinkedList_InsertNewHead(struct Node *current_head, struct Node * new_node)
{
    struct Node* new_head = NULL;

    if(current_head != NULL 
        && new_node != NULL
        && ezLinkedList_AppendNode(new_node, current_head->prev) == true)
    {
        new_head = new_node;
    }

    return new_head;
}


struct Node * ezLinkedList_UnlinkCurrentHead(struct Node * head)
{
    struct Node* new_head = head;

    if(head->next != head)
    {
        new_head = head->next;
        EZ_LINKEDLIST_UNLINK_NODE(head);
    }

    return new_head;
}


bool ezLinkedList_IsNodeInList(struct Node* head, struct Node* searched_node)
{
    bool is_existing = false;
    struct Node* node;

    if (head == searched_node)
    {
        is_existing = true;
    }
    else if (head != NULL && searched_node != NULL)
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



#if 0
Node* ezmLL_GetFreeNode(void)
{
    Node *free_node = NULL;

    for (uint16_t i = 0; i < NUM_OF_NODE; i++)
    {
        if (node_pool[i].u16NodeIndex == NODE_INVALID_ID)
        {
            /* store its own index for eaiser look up*/
            node_pool[i].u16NodeIndex = i;

            free_node = &node_pool[i];
            break;
        }
    }

    return free_node;
}

void ezmLL_ResetNode(Node * node)
{
    if (NULL != node && node->u16NodeIndex < NUM_OF_NODE)
    {
        node->pstNextNode = NULL;
        node->pstPrevNode = NULL;
        node->u16NodeIndex = NODE_INVALID_ID;
    }
}
#endif

#endif /* (EZ_LINKEDLIST == 1U) */
/* End of file*/
