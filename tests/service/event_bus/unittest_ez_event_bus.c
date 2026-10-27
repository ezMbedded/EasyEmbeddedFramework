/*****************************************************************************
* Filename:         unittest_ez_event_notifier.c
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

/** @file   unittest_ez_event_notifier.c
 *  @author Hai Nguyen
 *  @date   06.03.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "unity.h"
#include "unity_fixture.h"
#include "ez_event_bus.h"

TEST_GROUP(ez_event_bus);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define NOTIFY_CODE_1           1
#define NOTIFY_CODE_2           2
#define NOTIFY_CODE_3           3
#define NUM_OF_TEST_OBSERVER    2
#define MAX_RECORDED_EVENTS     8
#define MAX_RECORDED_DATA       64


/******************************************************************************
* Module Typedefs
*******************************************************************************/
typedef struct{
    int a;
    int b;
}TestData_t;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static ezEventBus_t test_subject;
static ezEventListener_t listener1;
static ezEventListener_t listener2;

typedef struct{
    uint32_t num_of_calls;
    uint32_t codes[MAX_RECORDED_EVENTS];
    size_t sizes[MAX_RECORDED_EVENTS];
    bool is_data_null[MAX_RECORDED_EVENTS];
    uint8_t data[MAX_RECORDED_EVENTS][MAX_RECORDED_DATA];
}Record_t;

static Record_t record1;
static Record_t record2;

/* When set, the first callback receiving NOTIFY_CODE_1 sends NOTIFY_CODE_2 */
static bool send_from_callback;
static TestData_t nested_data;
static bool nested_send_result;

/* Number of times the listener 1 sends NOTIFY_CODE_1 again when it receives it */
static uint32_t resend_budget;

static uint8_t buff[1024];

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
int Listener1_Callback(uint32_t event_code, const void *data, size_t data_size);
int Listener2_Callback(uint32_t event_code, const void *data, size_t data_size);
static void Record(Record_t *record, uint32_t event_code, const void *data, size_t data_size);


/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_event_bus)
{
    bool success = true;

    success = (ezSUCCESS == ezEventBus_CreateBus(&test_subject, buff, sizeof(buff)));
    success &= (ezSUCCESS == ezEventBus_CreateListener(&listener1, Listener1_Callback));
    success &= (ezSUCCESS == ezEventBus_CreateListener(&listener2, Listener2_Callback));

    if (success)
    {
        memset(&record1, 0, sizeof(record1));
        memset(&record2, 0, sizeof(record2));
        send_from_callback = false;
        nested_send_result = false;
        resend_budget = 0;
        success &= (ezSUCCESS == ezEventBus_Listen(&test_subject, &listener1));
        success &= (ezSUCCESS == ezEventBus_Listen(&test_subject, &listener2));
    }
}


TEST_TEAR_DOWN(ez_event_bus)
{
    ezEventBus_ResetBus(&test_subject);
}


TEST_GROUP_RUNNER(ez_event_bus)
{
    RUN_TEST_CASE(ez_event_bus, SubscribeToSubject);
    RUN_TEST_CASE(ez_event_bus, UnsubscribeFromSubject);
    RUN_TEST_CASE(ez_event_bus, ResetBus);
    RUN_TEST_CASE(ez_event_bus, NotifyEvent1);
    RUN_TEST_CASE(ez_event_bus, SendEventWithoutData);
    RUN_TEST_CASE(ez_event_bus, SendEventNullDataWithSize);
    RUN_TEST_CASE(ez_event_bus, SendEventZeroSizeWithData);
    RUN_TEST_CASE(ez_event_bus, EventsAreDeliveredInOrder);
    RUN_TEST_CASE(ez_event_bus, RunWithEmptyQueue);
    RUN_TEST_CASE(ez_event_bus, RunWithoutListeners);
    RUN_TEST_CASE(ez_event_bus, SendEventInvalidArgument);
    RUN_TEST_CASE(ez_event_bus, SendEventQueueFull);
    RUN_TEST_CASE(ez_event_bus, SendEventFromCallback);
    RUN_TEST_CASE(ez_event_bus, RunReturnsWhenCallbackResendsEvent);
    RUN_TEST_CASE(ez_event_bus, ListenTwiceIsRejected);
    RUN_TEST_CASE(ez_event_bus, ResetBusUnlinksListenersAndDropsEvents);
}


TEST(ez_event_bus, SubscribeToSubject)
{
    uint16_t num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, NUM_OF_TEST_OBSERVER);
}


TEST(ez_event_bus, UnsubscribeFromSubject)
{
    uint16_t num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, NUM_OF_TEST_OBSERVER);

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Unlisten(&test_subject, &listener1));
    num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, NUM_OF_TEST_OBSERVER - 1);

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Unlisten(&test_subject, &listener2));
    num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, 0);
}


TEST(ez_event_bus, ResetBus)
{
    uint16_t num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, NUM_OF_TEST_OBSERVER);

    ezEventBus_ResetBus(&test_subject);
    num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, 0);
}


TEST(ez_event_bus, NotifyEvent1)
{
    TestData_t test_data = { .a = 10, .b = 20 };
    uint16_t num_of_observers = ezEventBus_GetNumOfListeners(&test_subject);
    TEST_ASSERT_EQUAL(num_of_observers, NUM_OF_TEST_OBSERVER);

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(test_data)));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));

    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record1.codes[0]);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record2.codes[0]);
    TEST_ASSERT_EQUAL(sizeof(test_data), record1.sizes[0]);
    TEST_ASSERT_EQUAL(sizeof(test_data), record2.sizes[0]);
    TEST_ASSERT_EQUAL_MEMORY(&test_data, record1.data[0], sizeof(test_data));
    TEST_ASSERT_EQUAL_MEMORY(&test_data, record2.data[0], sizeof(test_data));
}


TEST(ez_event_bus, SendEventWithoutData)
{
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));

    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record1.codes[0]);
    TEST_ASSERT_EQUAL(0, record1.sizes[0]);
    TEST_ASSERT_TRUE(record1.is_data_null[0]);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);
    TEST_ASSERT_EQUAL(0, record2.sizes[0]);
    TEST_ASSERT_TRUE(record2.is_data_null[0]);
}


TEST(ez_event_bus, SendEventNullDataWithSize)
{
    /* No data pointer means an empty event, whatever the size is */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 16));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));

    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record1.codes[0]);
    TEST_ASSERT_EQUAL(0, record1.sizes[0]);
    TEST_ASSERT_TRUE(record1.is_data_null[0]);
}


TEST(ez_event_bus, SendEventZeroSizeWithData)
{
    TestData_t test_data = { .a = 1, .b = 2 };

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));

    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(0, record1.sizes[0]);
    TEST_ASSERT_TRUE(record1.is_data_null[0]);
}


TEST(ez_event_bus, EventsAreDeliveredInOrder)
{
    TestData_t data_a = { .a = 1, .b = 2 };
    uint8_t data_b = 0x5A;

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &data_a, sizeof(data_a)));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_2, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_3, &data_b, sizeof(data_b)));

    /* One run delivers every queued event */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(3, record1.num_of_calls);
    TEST_ASSERT_EQUAL(3, record2.num_of_calls);

    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record1.codes[0]);
    TEST_ASSERT_EQUAL(sizeof(data_a), record1.sizes[0]);
    TEST_ASSERT_EQUAL_MEMORY(&data_a, record1.data[0], sizeof(data_a));

    TEST_ASSERT_EQUAL(NOTIFY_CODE_2, record1.codes[1]);
    TEST_ASSERT_EQUAL(0, record1.sizes[1]);
    TEST_ASSERT_TRUE(record1.is_data_null[1]);

    TEST_ASSERT_EQUAL(NOTIFY_CODE_3, record1.codes[2]);
    TEST_ASSERT_EQUAL(sizeof(data_b), record1.sizes[2]);
    TEST_ASSERT_EQUAL_MEMORY(&data_b, record1.data[2], sizeof(data_b));

    /* The queue is drained, nothing more is delivered */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(3, record1.num_of_calls);
}


TEST(ez_event_bus, RunWithEmptyQueue)
{
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(0, record1.num_of_calls);
    TEST_ASSERT_EQUAL(0, record2.num_of_calls);
}


TEST(ez_event_bus, RunWithoutListeners)
{
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Unlisten(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Unlisten(&test_subject, &listener2));

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(0, record1.num_of_calls);

    /* The event is consumed even with nobody listening */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Listen(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(0, record1.num_of_calls);
}


TEST(ez_event_bus, SendEventInvalidArgument)
{
    TestData_t test_data = { .a = 1, .b = 2 };

    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, ezEventBus_SendEvent(NULL, NOTIFY_CODE_1, &test_data, sizeof(test_data)));
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, ezEventBus_Run(NULL));

    /* Data bigger than what the queue can ever hold */
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, SIZE_MAX));
    TEST_ASSERT_EQUAL(ezFAIL, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(buff) * 2U));
}


TEST(ez_event_bus, SendEventQueueFull)
{
    TestData_t test_data = { .a = 3, .b = 4 };
    uint32_t num_of_sent = 0;

    while(num_of_sent < sizeof(buff)
          && ezSUCCESS == ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(test_data)))
    {
        num_of_sent++;
    }

    TEST_ASSERT_GREATER_THAN_UINT32(0, num_of_sent);
    TEST_ASSERT_LESS_THAN_UINT32(sizeof(buff), num_of_sent);

    /* Everything accepted before the queue was full is still delivered intact */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(num_of_sent, record1.num_of_calls);
    TEST_ASSERT_EQUAL(num_of_sent, record2.num_of_calls);
    for(uint32_t i = 0; i < MAX_RECORDED_EVENTS && i < num_of_sent; i++)
    {
        TEST_ASSERT_EQUAL(sizeof(test_data), record1.sizes[i]);
        TEST_ASSERT_EQUAL_MEMORY(&test_data, record1.data[i], sizeof(test_data));
    }

    /* The queue accepts events again once it has been drained */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(test_data)));
}


TEST(ez_event_bus, SendEventFromCallback)
{
    TestData_t test_data = { .a = 10, .b = 20 };

    /* Same size as the first event, so a reused block would be overwritten */
    nested_data.a = 0x5A5A5A5A;
    nested_data.b = 0x3C3C3C3C;
    send_from_callback = true;

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(test_data)));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));

    /* The event sent from the callback was accepted ... */
    TEST_ASSERT_TRUE(nested_send_result);

    /* ... but is not delivered by the current run */
    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);

    /* Both listeners, including the one notified after the sender, see the original payload */
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record1.codes[0]);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, record2.codes[0]);
    TEST_ASSERT_EQUAL(sizeof(test_data), record1.sizes[0]);
    TEST_ASSERT_EQUAL(sizeof(test_data), record2.sizes[0]);
    TEST_ASSERT_EQUAL_MEMORY(&test_data, record1.data[0], sizeof(test_data));
    TEST_ASSERT_EQUAL_MEMORY(&test_data, record2.data[0], sizeof(test_data));

    /* The nested event is delivered intact by the next run, exactly once */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(2, record1.num_of_calls);
    TEST_ASSERT_EQUAL(2, record2.num_of_calls);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_2, record1.codes[1]);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_2, record2.codes[1]);
    TEST_ASSERT_EQUAL(sizeof(nested_data), record1.sizes[1]);
    TEST_ASSERT_EQUAL(sizeof(nested_data), record2.sizes[1]);
    TEST_ASSERT_EQUAL_MEMORY(&nested_data, record1.data[1], sizeof(nested_data));
    TEST_ASSERT_EQUAL_MEMORY(&nested_data, record2.data[1], sizeof(nested_data));

    /* The queue is drained */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(2, record1.num_of_calls);
    TEST_ASSERT_EQUAL(2, record2.num_of_calls);
}


TEST(ez_event_bus, RunReturnsWhenCallbackResendsEvent)
{
    /* The budget keeps a broken Run from looping forever, it never reaches 0 in a correct one */
    resend_budget = 5;

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0));

    /* Each run delivers only what was queued when it started */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);
    TEST_ASSERT_EQUAL(4, resend_budget);

    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(2, record1.num_of_calls);
    TEST_ASSERT_EQUAL(2, record2.num_of_calls);
    TEST_ASSERT_EQUAL(3, resend_budget);

    /* Stop resending: the event left over by the last run is delivered, then the queue is empty */
    resend_budget = 0;
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(3, record1.num_of_calls);
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(3, record1.num_of_calls);
}


TEST(ez_event_bus, ListenTwiceIsRejected)
{
    TEST_ASSERT_EQUAL(ezFAIL, ezEventBus_Listen(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(NUM_OF_TEST_OBSERVER, ezEventBus_GetNumOfListeners(&test_subject));

    /* The listener is notified once, the list is not corrupted */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);

    /* Listening after unlistening is accepted again */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Unlisten(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Listen(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(NUM_OF_TEST_OBSERVER, ezEventBus_GetNumOfListeners(&test_subject));
}


TEST(ez_event_bus, ResetBusUnlinksListenersAndDropsEvents)
{
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_2, NULL, 0));

    ezEventBus_ResetBus(&test_subject);

    /* The listeners are unlinked, not left pointing into the bus */
    TEST_ASSERT_EQUAL(0, ezEventBus_GetNumOfListeners(&test_subject));
    TEST_ASSERT_TRUE(listener1.node.next == &listener1.node);
    TEST_ASSERT_TRUE(listener1.node.prev == &listener1.node);
    TEST_ASSERT_TRUE(listener2.node.next == &listener2.node);
    TEST_ASSERT_TRUE(listener2.node.prev == &listener2.node);

    /* The waiting events are dropped */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Listen(&test_subject, &listener1));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(0, record1.num_of_calls);

    /* The bus and the listeners work again after the reset */
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Listen(&test_subject, &listener2));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_3, NULL, 0));
    TEST_ASSERT_EQUAL(ezSUCCESS, ezEventBus_Run(&test_subject));
    TEST_ASSERT_EQUAL(1, record1.num_of_calls);
    TEST_ASSERT_EQUAL(1, record2.num_of_calls);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_3, record1.codes[0]);
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_event_bus);
}


int Listener1_Callback(uint32_t event_code, const void *data, size_t data_size)
{
    Record(&record1, event_code, data, data_size);

    if(resend_budget > 0U && event_code == NOTIFY_CODE_1)
    {
        resend_budget--;
        (void)ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, NULL, 0);
    }
    return 0;
}


int Listener2_Callback(uint32_t event_code, const void *data, size_t data_size)
{
    Record(&record2, event_code, data, data_size);
    return 0;
}


static void Record(Record_t *record, uint32_t event_code, const void *data, size_t data_size)
{
    uint32_t i = record->num_of_calls;

    if(send_from_callback && event_code == NOTIFY_CODE_1)
    {
        /* Only the first listener notified sends the event */
        send_from_callback = false;
        nested_send_result = (ezSUCCESS == ezEventBus_SendEvent(
            &test_subject,
            NOTIFY_CODE_2,
            &nested_data,
            sizeof(nested_data)));
    }

    record->num_of_calls++;
    if(i < MAX_RECORDED_EVENTS)
    {
        record->codes[i] = event_code;
        record->sizes[i] = data_size;
        record->is_data_null[i] = (data == NULL);
        if(data != NULL && data_size <= MAX_RECORDED_DATA)
        {
            memcpy(record->data[i], data, data_size);
        }
    }
}


/* End of file */
