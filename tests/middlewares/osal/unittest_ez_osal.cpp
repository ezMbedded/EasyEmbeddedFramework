/*****************************************************************************
* Filename:         unittest_ez_osal.c
* Author:           Hai Nguyen
* Original Date:    15.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_osal.c
 *  @author Hai Nguyen
 *  @date   15.03.2025
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
#include "ez_osal.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(ezSTATUS, Init, void *);
FAKE_VALUE_FUNC(ezSTATUS, TaskCreate, ezOsal_TaskHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TaskDelete, ezOsal_TaskHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TaskSuspend, ezOsal_TaskHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TaskResume, ezOsal_TaskHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TaskDelay, unsigned long);


FAKE_VALUE_FUNC(ezSTATUS, SemaphoreCreate, ezOsal_SemaphoreHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, SemaphoreDelete, ezOsal_SemaphoreHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, SemaphoreGive, ezOsal_SemaphoreHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, SemaphoreTake, ezOsal_SemaphoreHandle_t*, uint32_t);

FAKE_VALUE_FUNC(ezSTATUS, TimerCreate, ezOsal_TimerHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TimerDelete, ezOsal_TimerHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TimerStart, ezOsal_TimerHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, TimerStop, ezOsal_TimerHandle_t*);

FAKE_VALUE_FUNC(ezSTATUS, EventCreate, ezOsal_EventHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, EventDelete, ezOsal_EventHandle_t*);
FAKE_VALUE_FUNC(ezSTATUS, EventWait, ezOsal_EventHandle_t*, uint32_t, uint32_t);
FAKE_VALUE_FUNC(ezSTATUS, EventSet, ezOsal_EventHandle_t*, uint32_t);
FAKE_VALUE_FUNC(ezSTATUS, EventClear, ezOsal_EventHandle_t*, uint32_t);

/******************************************************************************
* Module Typedefs
*******************************************************************************/
EZ_OSAL_DEFINE_TASK_HANDLE(MockTask, 1024, 1, NULL, NULL, NULL);
EZ_OSAL_DEFINE_SEMAPHORE_HANDLE(MockSemaphore, 2, NULL);
EZ_OSAL_DEFINE_TIMER_HANDLE(MockTimer, 1000, NULL, NULL, NULL);
EZ_OSAL_DEFINE_EVENT_HANDLE(MockEvent, NULL);


class OsalTestsFixture {
private:
protected:
    ezOsal_Interfaces_t test_interface;
public:
    OsalTestsFixture();
    ~OsalTestsFixture(){}
protected:
};


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/


/******************************************************************************
* Function Definitions
*******************************************************************************/


/******************************************************************************
* External functions
*******************************************************************************/

TEST_CASE_METHOD(OsalTestsFixture, "Test no interface is implemented", "[middlewares][osal]")
{
    CHECK(ezOsal_Init(NULL) == ezFAIL);

    CHECK(ezOsal_TaskCreate(&MockTask) == ezFAIL);

    CHECK(ezOsal_TaskDelete(&MockTask) == ezFAIL);
    CHECK(ezOsal_TaskSuspend(&MockTask) == ezFAIL);
    CHECK(ezOsal_TaskResume(&MockTask) == ezFAIL);

    CHECK(ezOsal_SemaphoreCreate(&MockSemaphore) == ezFAIL);

    CHECK(ezOsal_SemaphoreDelete(&MockSemaphore) == ezFAIL);
    CHECK(ezOsal_SemaphoreTake(&MockSemaphore, 100) == ezFAIL);
    CHECK(ezOsal_SemaphoreGive(&MockSemaphore) == ezFAIL);

    CHECK(ezOsal_TimerCreate(&MockTimer) == ezFAIL);
    CHECK(ezOsal_TimerDelete(&MockTimer) == ezFAIL);
    CHECK(ezOsal_TimerStart(&MockTimer) == ezFAIL);
    CHECK(ezOsal_TimerStop(&MockTimer) == ezFAIL);
}


TEST_CASE_METHOD(OsalTestsFixture, "Test task functions", "[middlewares][osal][task]")
{
    ezSTATUS status = ezFAIL;
    status = ezOsal_SetInterface(&test_interface);
    REQUIRE(status == ezSUCCESS);

    SECTION("ezOsal_pfInit")
    {
        Init_fake.return_val = ezFAIL;
        status = Init(NULL);
        CHECK(status == ezFAIL);
        CHECK(Init_fake.call_count == 1);

        Init_fake.return_val = ezSUCCESS;
        status = Init(NULL);
        CHECK(status == ezSUCCESS);
        CHECK(Init_fake.call_count == 2);
    }

    SECTION("ezOsal_fpTaskCreate")
    {
        TaskCreate_fake.return_val = ezFAIL;
        CHECK(TaskCreate(&MockTask) == ezFAIL);
        CHECK(TaskCreate_fake.call_count == 1);

        TaskCreate_fake.return_val = ezSUCCESS;
        CHECK(TaskCreate(&MockTask) == ezSUCCESS);
        CHECK(TaskCreate_fake.call_count == 2);
    }

    SECTION("ezOsal_fpTaskDelete")
    {
        TaskDelete_fake.return_val = ezSUCCESS;
        status = TaskDelete(&MockTask);
        CHECK(status == ezSUCCESS);
        CHECK(TaskDelete_fake.call_count == 1);
    }

    SECTION("ezOsal_fpTaskResume")
    {
        TaskResume_fake.return_val = ezSUCCESS;
        status = TaskResume(&MockTask);
        CHECK(status == ezSUCCESS);
        CHECK(TaskResume_fake.call_count == 1);
    }

    SECTION("TaskSuspend")
    {
        TaskSuspend_fake.return_val = ezSUCCESS;
        status = TaskSuspend(&MockTask);
        CHECK(status == ezSUCCESS);
        CHECK(TaskSuspend_fake.call_count == 1);
    }
}


TEST_CASE_METHOD(OsalTestsFixture, "Test semaphore functions", "[middlewares][osal][semaphore]")
{
    ezSTATUS status = ezFAIL;
    status = ezOsal_SetInterface(&test_interface);
    REQUIRE(status == ezSUCCESS);

    SECTION("ezOsal_fpSemaphoreCreate")
    {
        SemaphoreCreate_fake.return_val = ezFAIL;
        CHECK(ezFAIL == SemaphoreCreate(&MockSemaphore));
        CHECK(SemaphoreCreate_fake.call_count == 1);

        SemaphoreCreate_fake.return_val = ezSUCCESS;
        CHECK(ezSUCCESS == SemaphoreCreate(&MockSemaphore));
        CHECK(SemaphoreCreate_fake.call_count == 2);
    }

    SECTION("ezOsal_fpSemaphoreDelete")
    {
        SemaphoreDelete_fake.return_val = ezSUCCESS;
        status = SemaphoreDelete(&MockSemaphore);
        CHECK(status == ezSUCCESS);
        CHECK(SemaphoreDelete_fake.call_count == 1);
    }

    SECTION("ezOsal_fpSemaphoreTake")
    {
        SemaphoreTake_fake.return_val = ezSUCCESS;
        status = SemaphoreTake(&MockSemaphore, 100);
        CHECK(status == ezSUCCESS);
        CHECK(SemaphoreTake_fake.call_count == 1);
    }

    SECTION("ezOsal_fpSemaphoreGive")
    {
        SemaphoreGive_fake.return_val = ezSUCCESS;
        status = SemaphoreGive(&MockSemaphore);
        CHECK(status == ezSUCCESS);
        CHECK(SemaphoreGive_fake.call_count == 1);
    }
}


TEST_CASE_METHOD(OsalTestsFixture, "Test timer functions", "[middlewares][osal][timer]")
{
    ezSTATUS status = ezFAIL;

    SECTION("ezOsal_fpEventCreate")
    {
        EventCreate_fake.return_val = ezFAIL;
        CHECK(ezFAIL == EventCreate(&MockEvent));
        CHECK(EventCreate_fake.call_count == 1);

        EventCreate_fake.return_val = ezSUCCESS;
        CHECK(ezSUCCESS == EventCreate(&MockEvent));
        CHECK(EventCreate_fake.call_count == 2);
    }

    SECTION("ezOsal_fpEventDelete")
    {
        EventDelete_fake.return_val = ezSUCCESS;
        status = EventDelete(&MockEvent);
        CHECK(status == ezSUCCESS);
        CHECK(EventDelete_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventWait")
    {
        EventWait_fake.return_val = ezSUCCESS;
        status = EventWait(&MockEvent, 0x01, 100);
        CHECK(status == ezSUCCESS);
        CHECK(EventWait_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventSet")
    {
        EventSet_fake.return_val = ezSUCCESS;
        status = EventSet(&MockEvent, 0x02);
        CHECK(status == ezSUCCESS);
        CHECK(EventSet_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventClear")
    {
        EventClear_fake.return_val = ezSUCCESS;
        status = EventClear(&MockEvent, 0x02);
        CHECK(status == ezSUCCESS);
        CHECK(EventClear_fake.call_count == 1);
    }
}


TEST_CASE_METHOD(OsalTestsFixture, "Test event functions", "[middlewares][osal][event]")
{
    ezSTATUS status = ezFAIL;

    SECTION("ezOsal_fpEventCreate")
    {
        TimerCreate_fake.return_val = ezFAIL;
        CHECK(ezFAIL == TimerCreate(&MockTimer));
        CHECK(TimerCreate_fake.call_count == 1);

        TimerCreate_fake.return_val = ezSUCCESS;
        CHECK(ezSUCCESS == TimerCreate(&MockTimer));
        CHECK(TimerCreate_fake.call_count == 2);
    }

    SECTION("ezOsal_fpEventDelete")
    {
        TimerDelete_fake.return_val = ezSUCCESS;
        status = TimerDelete(&MockTimer);
        CHECK(status == ezSUCCESS);
        CHECK(TimerDelete_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventWait")
    {
        TimerStart_fake.return_val = ezSUCCESS;
        status = TimerStart(&MockTimer);
        CHECK(status == ezSUCCESS);
        CHECK(TimerStart_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventSet")
    {
        TimerStop_fake.return_val = ezSUCCESS;
        status = TimerStop(&MockTimer);
        CHECK(status == ezSUCCESS);
        CHECK(TimerStop_fake.call_count == 1);
    }

    SECTION("ezOsal_fpEventClear")
    {
        TimerStop_fake.return_val = ezSUCCESS;
        status = TimerStop(&MockTimer);
        CHECK(status == ezSUCCESS);
        CHECK(TimerStop_fake.call_count == 1);
    }
}



/******************************************************************************
* Internal functions
*******************************************************************************/
OsalTestsFixture::OsalTestsFixture()
{
    RESET_FAKE(Init);
    RESET_FAKE(TaskCreate);
    RESET_FAKE(TaskDelete);
    RESET_FAKE(TaskSuspend);
    RESET_FAKE(TaskResume);
    RESET_FAKE(SemaphoreCreate);
    RESET_FAKE(SemaphoreDelete);
    RESET_FAKE(SemaphoreGive);
    RESET_FAKE(SemaphoreTake);
    RESET_FAKE(TimerCreate);
    RESET_FAKE(TimerDelete);
    RESET_FAKE(TimerStart);
    RESET_FAKE(TimerStop);

    test_interface.Init = Init,
    test_interface.TaskCreate = TaskCreate;
    test_interface.TaskDelete = TaskDelete;
    test_interface.TaskSuspend = TaskSuspend;
    test_interface.TaskResume = TaskResume;

    test_interface.SemaphoreCreate = SemaphoreCreate;
    test_interface.SemaphoreDelete = SemaphoreDelete;
    test_interface.SemaphoreGive = SemaphoreGive;
    test_interface.SemaphoreTake = SemaphoreTake;
    
    test_interface.TimerCreate = TimerCreate;
    test_interface.TimerDelete = TimerDelete;
    test_interface.TimerStart = TimerStart;
    test_interface.TimerStop = TimerStop;
}


/* End of file */
