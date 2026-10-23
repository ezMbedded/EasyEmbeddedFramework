/*****************************************************************************
* Filename:         unittest_ez_linked_list.c
* Author:           Hai Nguyen
* Original Date:    06.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_linked_list.c
 *  @author Hai Nguyen
 *  @date   06.03.2024
 *  @brief  Unit test for linked list component
 *
 *  @details
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "unity.h"
#include "unity_fixture.h"
#include "ez_linked_list.h"

TEST_GROUP(ez_linked_list);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
/* None */

/******************************************************************************
* Module Typedefs
*******************************************************************************/
struct test_struct1
{
    char c;
    struct Node node;
};

struct a_more_complex_one
{
    char c;
    uint8_t buffer[5];
    uint32_t data;
    struct Node node;
    bool busy;
};

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);

/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_linked_list)
{
}


TEST_TEAR_DOWN(ez_linked_list)
{
}


TEST_GROUP_RUNNER(ez_linked_list)
{
    RUN_TEST_CASE(ez_linked_list, Test_Macro);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_InsertHead);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_InsertTail);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_RemoveFirst);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_IsNodeInList);
    RUN_TEST_CASE(ez_linked_list, Unlink_node);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_AppendNode);
    RUN_TEST_CASE(ez_linked_list, EZ_LINKEDLIST_ADD_HEAD);
}


TEST(ez_linked_list, Test_Macro)
{
    struct Node head = EZ_LINKEDLIST_INIT_NODE(head);
    
    TEST_ASSERT_EQUAL(head.next, &head);
    TEST_ASSERT_EQUAL(head.prev, &head);

    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    TEST_ASSERT_EQUAL(node1.next, &node1);
    TEST_ASSERT_EQUAL(node1.prev, &node1);

    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    TEST_ASSERT_EQUAL(node2.next, &node2);
    TEST_ASSERT_EQUAL(node2.prev, &node2);

    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    TEST_ASSERT_EQUAL(node3.next, &node3);
    TEST_ASSERT_EQUAL(node3.prev, &node3);

    EZ_LINKEDLIST_ADD_HEAD(&head, &node1);
    EZ_LINKEDLIST_ADD_TAIL(&head, &node2);

    struct Node* it_node = NULL;
    uint32_t count = 0;

    EZ_LINKEDLIST_FOR_EACH(it_node, &head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 2);

    EZ_LINKEDLIST_UNLINK_NODE(&node2);

    count = 0;
    EZ_LINKEDLIST_FOR_EACH(it_node, &head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 1);

    it_node = &head;
    EZ_LINKEDLIST_TO_NEXT_NODE(it_node);
    TEST_ASSERT_EQUAL(it_node, &node1);

    struct test_struct1 data1;
    data1.c = 'a';

    struct test_struct1* test1 = EZ_LINKEDLIST_GET_PARENT_OF(&data1.node, node, struct test_struct1);
    TEST_ASSERT_EQUAL(test1, &data1);
    TEST_ASSERT_EQUAL(test1->c, data1.c);

    struct a_more_complex_one complex_one;
    complex_one.busy = false;
    complex_one.data = 123456;

    struct a_more_complex_one * complex_one_ptr = EZ_LINKEDLIST_GET_PARENT_OF(&complex_one.node, node, struct a_more_complex_one);
    TEST_ASSERT_EQUAL(complex_one_ptr, &complex_one);
    TEST_ASSERT_EQUAL(complex_one_ptr->c, complex_one.c);
    TEST_ASSERT_EQUAL(complex_one_ptr->busy, complex_one.busy);
    TEST_ASSERT_EQUAL(complex_one_ptr->data, complex_one.data);
}

TEST(ez_linked_list, ezLinkedList_InsertHead)
{
    uint32_t count = 0;
    struct Node *iterate_node;

    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    void* address[5] =
    {
        &node1,
        &node2,
        &node3,
        &node4,
        &node5,
    };

    struct Node list = EZ_LINKEDLIST_INIT_NODE(list);

    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertHead(NULL, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertHead(&node1, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertHead(NULL, &node2));

    EZ_LINKEDLIST_FOR_EACH(iterate_node, &list)
    {
        count = count + 1;
    }

    TEST_ASSERT_EQUAL(0, count);

    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertHead(&list, &node1));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertHead(&list, &node2));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertHead(&list, &node3));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertHead(&list, &node4));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertHead(&list, &node5));

    count = 0U;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, &list)
    {
        TEST_ASSERT_EQUAL(iterate_node, address[4 - count]);
        count = count + 1;
    }
    TEST_ASSERT_EQUAL(5, count);
}


TEST(ez_linked_list, ezLinkedList_InsertTail)
{
    uint32_t count = 0;
    struct Node *iterate_node;

    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    void* address[5] =
    {
        &node1,
        &node2,
        &node3,
        &node4,
        &node5,
    };

    struct Node list = EZ_LINKEDLIST_INIT_NODE(list);

    /* invalid arguments */
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertTail(NULL, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertTail(&list, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertTail(NULL, &node1));
    TEST_ASSERT_EQUAL(true, IS_LIST_EMPTY(&list));

    /* insert into an empty list */
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertTail(&list, &node1));
    TEST_ASSERT_EQUAL(&node1, list.next);
    TEST_ASSERT_EQUAL(&node1, list.prev);
    TEST_ASSERT_EQUAL(&list, node1.next);
    TEST_ASSERT_EQUAL(&list, node1.prev);

    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertTail(&list, &node2));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertTail(&list, &node3));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertTail(&list, &node4));
    TEST_ASSERT_EQUAL(true, ezLinkedList_InsertTail(&list, &node5));

    /* the last inserted node is the tail, the first inserted node stays first */
    TEST_ASSERT_EQUAL(&node1, list.next);
    TEST_ASSERT_EQUAL(&node5, list.prev);

    /* forward order is insertion order (first in, first out) */
    count = 0U;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, &list)
    {
        TEST_ASSERT_EQUAL(address[count], iterate_node);
        count = count + 1;
    }
    TEST_ASSERT_EQUAL(5, count);

    /* backward links are consistent */
    count = 0U;
    for (iterate_node = list.prev; iterate_node != &list; iterate_node = iterate_node->prev)
    {
        TEST_ASSERT_EQUAL(address[4 - count], iterate_node);
        count = count + 1;
    }
    TEST_ASSERT_EQUAL(5, count);

    TEST_ASSERT_EQUAL(5, ezLinkedList_GetListSize(&list));
}


TEST(ez_linked_list, ezLinkedList_RemoveFirst)
{
    struct Node* removed_node;

    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    struct Node* nodes[5] = { &node5, &node4, &node3, &node2, &node1 };
    uint32_t i;

    struct Node list = EZ_LINKEDLIST_INIT_NODE(list);

    TEST_ASSERT_NULL(ezLinkedList_RemoveFirst(NULL));
    TEST_ASSERT_NULL(ezLinkedList_RemoveFirst(&list));

    ezLinkedList_InsertHead(&list, &node1);
    ezLinkedList_InsertHead(&list, &node2);
    ezLinkedList_InsertHead(&list, &node3);
    ezLinkedList_InsertHead(&list, &node4);
    ezLinkedList_InsertHead(&list, &node5);

    for (i = 0; i < 5; i++)
    {
        removed_node = ezLinkedList_RemoveFirst(&list);
        TEST_ASSERT_EQUAL_PTR(nodes[i], removed_node);

        /* removed node is re-initialized and no longer part of the list */
        TEST_ASSERT_EQUAL_PTR(removed_node, removed_node->next);
        TEST_ASSERT_EQUAL_PTR(removed_node, removed_node->prev);
        TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, removed_node));
        TEST_ASSERT_EQUAL(4 - i, ezLinkedList_GetListSize(&list));
    }

    TEST_ASSERT_TRUE(IS_LIST_EMPTY(&list));
    TEST_ASSERT_NULL(ezLinkedList_RemoveFirst(&list));
}

TEST(ez_linked_list, ezLinkedList_IsNodeInList)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);
    struct Node outside_node = EZ_LINKEDLIST_INIT_NODE(outside_node);

    struct Node list = EZ_LINKEDLIST_INIT_NODE(list);

    /* empty list */
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &node1));

    ezLinkedList_InsertHead(&list, &node1);
    ezLinkedList_InsertHead(&list, &node2);
    ezLinkedList_InsertHead(&list, &node3);
    ezLinkedList_InsertHead(&list, &node4);
    ezLinkedList_InsertHead(&list, &node5);

    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node1));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node2));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node3));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node4));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node5));

    /* a node that is not in the list */
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &outside_node));

    /* the sentinel is not a member of its own list */
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &list));

    /* invalid arguments */
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(NULL, NULL));
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, NULL));
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(NULL, &node1));
}

TEST(ez_linked_list, Unlink_node)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    struct Node list = EZ_LINKEDLIST_INIT_NODE(list);
    ezLinkedList_InsertTail(&list, &node1);
    ezLinkedList_InsertTail(&list, &node2);
    ezLinkedList_InsertTail(&list, &node3);
    ezLinkedList_InsertTail(&list, &node4);
    ezLinkedList_InsertTail(&list, &node5);
    TEST_ASSERT_EQUAL(5, ezLinkedList_GetListSize(&list));

    /* unlink a node in the middle, the others stay linked */
    EZ_LINKEDLIST_UNLINK_NODE(&node3);
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &node3));
    TEST_ASSERT_EQUAL_PTR(&node3, node3.next);
    TEST_ASSERT_EQUAL_PTR(&node3, node3.prev);
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node1));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node2));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node4));
    TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&list, &node5));
    TEST_ASSERT_EQUAL(4, ezLinkedList_GetListSize(&list));
    TEST_ASSERT_EQUAL_PTR(&node4, node2.next);
    TEST_ASSERT_EQUAL_PTR(&node2, node4.prev);

    /* unlink the first and the last node */
    EZ_LINKEDLIST_UNLINK_NODE(&node1);
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &node1));
    TEST_ASSERT_EQUAL_PTR(&node2, list.next);

    EZ_LINKEDLIST_UNLINK_NODE(&node5);
    TEST_ASSERT_FALSE(ezLinkedList_IsNodeInList(&list, &node5));
    TEST_ASSERT_EQUAL_PTR(&node4, list.prev);
    TEST_ASSERT_EQUAL(2, ezLinkedList_GetListSize(&list));

    /* unlinking a node that is not in a list is harmless */
    EZ_LINKEDLIST_UNLINK_NODE(&node3);
    TEST_ASSERT_EQUAL_PTR(&node3, node3.next);
    TEST_ASSERT_EQUAL(2, ezLinkedList_GetListSize(&list));

    EZ_LINKEDLIST_UNLINK_NODE(&node2);
    EZ_LINKEDLIST_UNLINK_NODE(&node4);
    TEST_ASSERT_TRUE(IS_LIST_EMPTY(&list));
}

TEST(ez_linked_list, ezLinkedList_AppendNode)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node* head = &node1;

    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node2));
    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node3));

    TEST_ASSERT_EQUAL(true, ezLinkedList_AppendNode(&node2, head));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node2));

    TEST_ASSERT_EQUAL(true, ezLinkedList_AppendNode(&node3, &node2));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node3));
}

TEST(ez_linked_list, EZ_LINKEDLIST_ADD_HEAD)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node* head = &node1;
    struct Node* iterate_node;
    uint8_t count = 0;

    TEST_ASSERT_EQUAL(true, IS_LIST_EMPTY(head));
    EZ_LINKEDLIST_ADD_HEAD(head, &node2);
    
    TEST_ASSERT_EQUAL(false, IS_LIST_EMPTY(head));
    
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node2));
    
    EZ_LINKEDLIST_FOR_EACH(iterate_node, head)
    {
        count++;
        if (iterate_node == &node2)
        {
            break;
        }
    }
    TEST_ASSERT_EQUAL(1, count);

    count = 0;
    EZ_LINKEDLIST_ADD_HEAD(head, &node3);
    EZ_LINKEDLIST_FOR_EACH(iterate_node, head)
    {
        count++;
        if (iterate_node == &node3)
        {
            break;
        }
    }

    TEST_ASSERT_EQUAL(1, count);
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_linked_list);
}


/* End of file */
