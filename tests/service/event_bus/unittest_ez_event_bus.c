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
#define NUM_OF_TEST_OBSERVER    2


/******************************************************************************
* Module Typedefs
*******************************************************************************/
typedef struct{
    int a;
    int b;;
}TestData_t;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static ezEventBus_t test_subject;
static ezEventListener_t listener1;
static ezEventListener_t listener2;

static uint32_t listener1_notiffy_code;
static uint32_t listener2_notiffy_code;

static uint8_t buff[1024];
static TestData_t data1;
static TestData_t data2;

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
int Listener1_Callback(uint32_t event_code, const void *data, size_t data_size);
int Listener2_Callback(uint32_t event_code, const void *data, size_t data_size);


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
        listener1_notiffy_code = 0;
        listener2_notiffy_code = 0;
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

    ezEventBus_SendEvent(&test_subject, NOTIFY_CODE_1, &test_data, sizeof(test_data));
    ezEventBus_Run(&test_subject);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, listener1_notiffy_code);
    TEST_ASSERT_EQUAL(NOTIFY_CODE_1, listener2_notiffy_code);
    TEST_ASSERT_EQUAL_MEMORY(&test_data, &data1, sizeof(test_data));
    TEST_ASSERT_EQUAL_MEMORY(&test_data, &data2, sizeof(test_data));
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
    listener1_notiffy_code = event_code;
    memcpy(&data1, data, data_size);
    return 0;
}


int Listener2_Callback(uint32_t event_code, const void *data, size_t data_size)
{
    listener2_notiffy_code = event_code;
    memcpy(&data2, data, data_size);
    return 0;
}


/* End of file */
