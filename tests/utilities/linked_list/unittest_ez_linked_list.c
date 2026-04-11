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
    RUN_TEST_CASE(ez_linked_list, LinkedList_InsertNewHead);
    RUN_TEST_CASE(ez_linked_list, LinkedList_UnlinkCurrentHead);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_IsNodeInList);
    RUN_TEST_CASE(ez_linked_list, Unlink_node);
    RUN_TEST_CASE(ez_linked_list, ezLinkedList_AppendNode);
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

TEST(ez_linked_list, LinkedList_InsertNewHead)
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

    struct Node* head = &node1;

    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertNewHead(NULL, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertNewHead(&node1, NULL));
    TEST_ASSERT_EQUAL(false, ezLinkedList_InsertNewHead(NULL, &node2));

    EZ_LINKEDLIST_FOR_EACH(iterate_node, head)
    {
        count = count + 1;
    }

    TEST_ASSERT_EQUAL(0, count);

    head = ezLinkedList_InsertNewHead(head, &node2);
    TEST_ASSERT_EQUAL(head, &node2);

    head = ezLinkedList_InsertNewHead(head, &node3);
    TEST_ASSERT_EQUAL(head, &node3);

    head = ezLinkedList_InsertNewHead(head, &node4);
    TEST_ASSERT_EQUAL(head, &node4);

    head = ezLinkedList_InsertNewHead(head, &node5);
    TEST_ASSERT_EQUAL(head, &node5);


    count = 0U;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, head)
    {
        TEST_ASSERT_EQUAL(iterate_node, address[3 - count]);
        count = count + 1;
    }
}


TEST(ez_linked_list, LinkedList_UnlinkCurrentHead)
{
    uint32_t count = 0;
    struct Node* iterate_node;
    struct Node* new_head;

    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    struct Node* head = &node1;
    head = ezLinkedList_InsertNewHead(head, &node2);
    head = ezLinkedList_InsertNewHead(head, &node3);
    head = ezLinkedList_InsertNewHead(head, &node4);
    head = ezLinkedList_InsertNewHead(head, &node5);

    new_head = ezLinkedList_UnlinkCurrentHead(head);
    TEST_ASSERT_EQUAL(new_head, &node4);
    TEST_ASSERT_EQUAL(head, &node5);

    count = 1;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, new_head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 4);

    head = &node4;
    new_head = ezLinkedList_UnlinkCurrentHead(new_head);
    TEST_ASSERT_EQUAL(new_head, &node3);
    TEST_ASSERT_EQUAL(head, &node4);

    count = 1;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, new_head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 3);

    head = &node3;
    new_head = ezLinkedList_UnlinkCurrentHead(head);
    TEST_ASSERT_EQUAL(new_head, &node2);
    TEST_ASSERT_EQUAL(head, &node3);

    count = 1;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, new_head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 2);

    head = &node2;
    new_head = ezLinkedList_UnlinkCurrentHead(head);
    TEST_ASSERT_EQUAL(new_head, &node1);
    TEST_ASSERT_EQUAL(head, &node2);

    count = 1;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, new_head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 1);

    head = &node1;
    new_head = ezLinkedList_UnlinkCurrentHead(head);
    TEST_ASSERT_EQUAL(new_head, &node1);
    TEST_ASSERT_EQUAL(head, &node1);

    count = 1;
    EZ_LINKEDLIST_FOR_EACH(iterate_node, new_head)
    {
        count++;
    }
    TEST_ASSERT_EQUAL(count, 1);
}

TEST(ez_linked_list, ezLinkedList_IsNodeInList)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    struct Node* head = &node1;
    head = ezLinkedList_InsertNewHead(head, &node2);
    head = ezLinkedList_InsertNewHead(head, &node3);
    head = ezLinkedList_InsertNewHead(head, &node4);
    head = ezLinkedList_InsertNewHead(head, &node5);

    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node1));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node2));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node3));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node4));
    TEST_ASSERT_EQUAL(true, ezLinkedList_IsNodeInList(head, &node5));

}

TEST(ez_linked_list, Unlink_node)
{
    struct Node node1 = EZ_LINKEDLIST_INIT_NODE(node1);
    struct Node node2 = EZ_LINKEDLIST_INIT_NODE(node2);
    struct Node node3 = EZ_LINKEDLIST_INIT_NODE(node3);
    struct Node node4 = EZ_LINKEDLIST_INIT_NODE(node4);
    struct Node node5 = EZ_LINKEDLIST_INIT_NODE(node5);

    struct Node* head = &node1;
    head = ezLinkedList_InsertNewHead(head, &node2);
    head = ezLinkedList_InsertNewHead(head, &node3);
    head = ezLinkedList_InsertNewHead(head, &node4);
    head = ezLinkedList_InsertNewHead(head, &node5);

    EZ_LINKEDLIST_UNLINK_NODE(&node1);
    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node1));

    EZ_LINKEDLIST_UNLINK_NODE(&node2);
    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node2));

    EZ_LINKEDLIST_UNLINK_NODE(&node3);
    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node3));

    EZ_LINKEDLIST_UNLINK_NODE(&node4);
    TEST_ASSERT_EQUAL(false, ezLinkedList_IsNodeInList(head, &node4));
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


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_linked_list);
}


/* End of file */
