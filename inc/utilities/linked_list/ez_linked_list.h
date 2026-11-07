/*****************************************************************************
* Filename:         ez_linked_list.h
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

/** @file   ez_linked_list.h
 *  @author Hai Nguyen
 *  @date   19.02.2024
 *  @brief  Public functions of the linked list component
 *
 *  @details Linked list data structure. This implementation is inspired by
 *  the one in the linux kernel.
 * 
 */

#ifndef EZ_LINKED_LIST_H 
#define EZ_LINKED_LIST_H 

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_LINKEDLIST == 1U)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ez_utilities_common.h"

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/

/** @brief Iterate through a linked list, starting from the node after head.
 *
 *  @details The loop ends when the iterator comes back to head. The list must
 *  not be modified while iterating.
 *
 *  @param node: Iterator variable of type struct Node *
 *  @param head: Pointer to the list head (sentinel)
 */
#define EZ_LINKEDLIST_FOR_EACH(node,head)     for(node = (head)->next; node != head; node = (node)->next)

/** @brief Static initializer of a node. The node points to itself.
 *
 *  @param name: Name of the node variable (not a pointer)
 */
#define EZ_LINKEDLIST_INIT_NODE(name)         {&(name), &(name)}

/** @brief Offset in bytes of a member inside a structure type.
 *
 *  @details It uses the same concept as the linked list of the linux kernel:
 *  (type*)0 casts address 0 (zero) to data type "type",
 *  ((type*)0)->member accesses the member of type "type", and the address of
 *  the member minus address 0 gives the offset.
 *
 *  @param type: Structure type
 *  @param member: Name of the member inside the structure
 */
#define OFFSET(type, member) ((char*)&(((type*)0)->member) - (char*)((type*)0))

/** @brief Get the parent of the data structure, where the linked list node is
 *  embedded.
 *
 *  @param ptr: Pointer to the embedded struct Node
 *  @param member: Name of the struct Node member inside the parent type
 *  @param type: Type of the parent structure
 */
#define EZ_LINKEDLIST_GET_PARENT_OF(ptr,member,type) (type*)((char*)ptr - OFFSET(type, member))

/** @brief Advance a node to the next node.
 *
 *  @param node: Node variable of type struct Node *, assigned the next node
 */
#define EZ_LINKEDLIST_TO_NEXT_NODE(node) node = node->next

/** @brief Insert a node to the head of a list. See ezLinkedList_InsertHead.
 *
 *  @param list_head: Pointer to the list head (sentinel)
 *  @param node: Pointer to the node to insert
 */
#define EZ_LINKEDLIST_ADD_HEAD(list_head,node) ezLinkedList_InsertHead(list_head, node)

/** @brief Insert a node to the tail of a list. See ezLinkedList_InsertTail.
 *
 *  @param list_head: Pointer to the list head (sentinel)
 *  @param node: Pointer to the node to insert
 */
#define EZ_LINKEDLIST_ADD_TAIL(list_head,node) ezLinkedList_InsertTail(list_head, node)

/** @brief Unlink a node from its list. See ezLinkedList_UnlinkNode.
 *
 *  @param node: Pointer to the node to unlink
 */
#define EZ_LINKEDLIST_UNLINK_NODE(node)     ezLinkedList_UnlinkNode(node)

/** @brief Check if a list is empty.
 *
 *  @param list_head: Pointer to the list head (sentinel)
 */
#define IS_LIST_EMPTY(list_head) ((list_head)->next == (list_head))

/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief Meta data of a node. Containing the pointer to the next node and
 *  previous node
 */
struct Node
{
    struct Node * next;  /**< pointer to the next node in a linked list*/
    struct Node * prev;  /**< pointer to the previous node in a linked list*/
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function : ezLinkedList_InitNode
*//** 
* @brief Initialize a new node
*
* @details The indication for new node is that, both head and tail point to
*          itself
*
* @param    node: (IN)pointer to node, must not be NULL
*
* @pre node is not NULL
* @post Node is initialized.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node new_node;
* ezLinkedList_InitNode(&new_node);
* @endcode
*
*
*****************************************************************************/
void ezLinkedList_InitNode (struct Node* node);


/*****************************************************************************
* Function : ezLinkedList_GetListSize
*//** 
* @brief Return number of node in a list
*
* @details This function transverses the linked list and count the number
* of nodes
*
* @param    list_head: (IN)pointer to the head of the list
* @return   number of node, 0 if list_head is NULL or the list is empty
*
* @pre None
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* uint16_t size = ezLinkedList_GetListSize(head);
* @endcode
*
*****************************************************************************/
uint16_t ezLinkedList_GetListSize(struct Node* list_head);


/*****************************************************************************
* Function : ezLinkedList_AppendNode
*//** 
* @brief Append a node after a node
*
* @details The new node is linked right after appended_node.
*
* @param    new_node: (IN)pointer to the new node
* @param    appended_node: (IN)pointer to the node, which new node will be
*                          appended to
* @return   true if success, else false
*
* @retval true   Node appended
* @retval false  new_node or appended_node is NULL
*
* @pre None
* @post On success, new_node follows appended_node.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node new_node;
* struct Node appended_node;
* ezLinkedList_InitNode(&new_node);
* ezLinkedList_InitNode(&appended_node);
* if (!ezLinkedList_AppendNode(&new_node, &appended_node))
* {
*     HandleError();
* }
* @endcode
*
* @see ezLinkedList_InitNode
*
*****************************************************************************/
bool ezLinkedList_AppendNode(struct Node *new_node, struct Node *appended_node);


/*****************************************************************************
* Function : ezLinkedList_InsertHead
*//** 
* @brief Add a new node to first position of the linked list
*
* @details The new node is inserted right after the list sentinel, so it
*          becomes the first node of the list
*
* @param    list: (IN)pointer to the linked list (sentinel)
* @param    new_node: (IN)pointer to the new node to be inserted
* @return   true if success, else false
*
* @retval true   Node inserted
* @retval false  list or new_node is NULL
*
* @pre None
* @post On success, new_node is the first node of the list.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node list;
* struct Node new_node;
* ezLinkedList_InitNode(&list);
* ezLinkedList_InitNode(&new_node);
* if (!ezLinkedList_InsertHead(&list, &new_node))
* {
*     HandleError();
* }
* @endcode
*
* @see ezLinkedList_InitNode
*
*****************************************************************************/
static inline bool ezLinkedList_InsertHead (struct Node * list, struct Node *new_node)
{
    return ezLinkedList_AppendNode(new_node, list);
}


/*****************************************************************************
* Function : ezLinkedList_InsertTail
*//** 
* @brief Add a new node to last position of the linked list
*
* @details The new node is inserted right before the list sentinel, so it
*          becomes the last node of the list
*
* @param    list: (IN)pointer to the linked list (sentinel)
* @param    new_node: (IN)pointer to the new node to be inserted
* @return   true if success, else false
*
* @retval true   Node inserted
* @retval false  list or new_node is NULL
*
* @pre None
* @post On success, new_node is the last node of the list.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node list;
* struct Node new_node;
* ezLinkedList_InitNode(&list);
* ezLinkedList_InitNode(&new_node);
* if (!ezLinkedList_InsertTail(&list, &new_node))
* {
*     HandleError();
* }
* @endcode
*
* @see ezLinkedList_InitNode
*
*****************************************************************************/
static inline bool ezLinkedList_InsertTail (struct Node * list, struct Node *new_node)
{
    return (list != NULL) && ezLinkedList_AppendNode(new_node, list->prev);
}


/*****************************************************************************
* Function : ezLinkedList_RemoveFirst
*//** 
* @brief Remove the first node from the linked list
*
* @details The first node is the one right after the list sentinel. The
*          removed node is re-initialized, so it points to itself, and the
*          list stays valid.
*
* @param    list: (IN)pointer to the linked list (sentinel)
* @return   pointer to the removed node or NULL if the list is empty
*
* @retval NULL  list is NULL or the list is empty
*
* @pre None
* @post On success, the removed node is no longer in the list.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node *removed_node = ezLinkedList_RemoveFirst(&list);
* if (removed_node == NULL)
* {
*     HandleEmptyList();
* }
* @endcode
*
*****************************************************************************/
struct Node * ezLinkedList_RemoveFirst(struct Node * list);


/*****************************************************************************
* Function : ezLinkedList_IsNodeInList
*//** 
* @brief Check if a node is in a list
*
* @details This function transverses the list starting after head and compares
* each node with searched_node.
*
* @param    head: (IN)pointer to head of the list
* @param    searched_node: (IN)node to be checked
* @return   true if existing, else false
*
* @retval true   searched_node is in the list
* @retval false  Node not found, or head or searched_node is NULL
*
* @pre None
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* bool exist = ezLinkedList_IsNodeInList(list, &checked_node);
* @endcode
*
*****************************************************************************/
bool ezLinkedList_IsNodeInList(struct Node *head, struct Node *searched_node);

/*****************************************************************************
 * Function : ezLinkedList_UnlinkNode
 *//**
* @brief Unlink a node from a list
* @details The node is unlinked from the list and re-initialized, so it points
* to itself. The list remains valid after the operation.
* @param    node: (IN)pointer to the node to be unlinked
*   
* @return   true if the node was successfully unlinked, false if the node was NULL
*
* @retval true   Node unlinked
* @retval false  node is NULL
*
* @pre The node is part of a list or initialized with ezLinkedList_InitNode.
* @post The node is unlinked from the list and re-initialized.
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct Node node;
* ezLinkedList_InitNode(&node);
* // Assume the node is part of a list
* if (!ezLinkedList_UnlinkNode(&node))
* {
*     HandleError();
* }
* @endcode
*
* @see ezLinkedList_InitNode
*
*****************************************************************************/
static inline bool ezLinkedList_UnlinkNode(struct Node *node)
{
    if (node != NULL)
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ezLinkedList_InitNode(node);
        return true;
    }
    return false;
}

#ifdef __cplusplus
}
#endif

#endif /* EZ_LINKEDLIST */
#endif /* EZ_LINKED_LIST_H  */

/* End of file*/
