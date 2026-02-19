/*****************************************************************************
* Filename:         unittest_ez_task_worker.c
* Author:           Hai Nguyen
* Original Date:    29.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_task_worker.c
 *  @author Hai Nguyen
 *  @date   29.03.2024
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
#include "unity.h"
#include "unity_fixture.h"
#include "ez_task_worker.h"

TEST_GROUP(ez_task_worker);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define BUFF_SIZE       256

/******************************************************************************
* Module Typedefs
*******************************************************************************/
struct Worker1SumContext
{
    int a;
    int b;
};

typedef enum
{
    WORKER1_EVENT_SUM_CMPLT,
    WORKER1_EVENT_ERROR,
}WORKER1_EVENT;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezTaskWorker worker1;
static struct ezTaskWorker worker2;
static uint8_t buff1[BUFF_SIZE];
static uint8_t buff2[BUFF_SIZE];
static int worker1_sum = 0;

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
static bool worker1_sum_external(int a, int b);
static bool worker1_sum_internal(void *context, ezTaskWorkerCallbackFunc callback);
static void callback1(uint8_t event, void *ret_data);

/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_task_worker)
{
    bool ret = false;

    ret = ezTaskWorker_CreateWorker(&worker1, buff1, BUFF_SIZE);
    TEST_ASSERT_EQUAL(true, ret);
    ret = ezTaskWorker_CreateWorker(&worker2, buff2, BUFF_SIZE);
    TEST_ASSERT_EQUAL(true, ret);
}


TEST_TEAR_DOWN(ez_task_worker)
{
}


TEST_GROUP_RUNNER(ez_task_worker)
{
    RUN_TEST_CASE(ez_task_worker, Test_ezTaskWorker_EnqueueTask);
}


TEST(ez_task_worker, Test_ezTaskWorker_EnqueueTask)
{
    bool ret = false;
    ret = worker1_sum_external(10, 12);
    TEST_ASSERT_EQUAL(true, ret);
    TEST_ASSERT_EQUAL(1, ezQueue_GetNumOfElement(&worker1.msg_queue));

    ret = worker1_sum_external(4, 5);
    TEST_ASSERT_EQUAL(true, ret);
    TEST_ASSERT_EQUAL(2, ezQueue_GetNumOfElement(&worker1.msg_queue));

    ret = worker1_sum_external(100, 200);
    TEST_ASSERT_EQUAL(true, ret);
    TEST_ASSERT_EQUAL(3, ezQueue_GetNumOfElement(&worker1.msg_queue));

    ezTaskWorker_ExecuteTaskNoRTOS();
    TEST_ASSERT_EQUAL(22, worker1_sum);
    TEST_ASSERT_EQUAL(2, ezQueue_GetNumOfElement(&worker1.msg_queue));

    ezTaskWorker_ExecuteTaskNoRTOS();
    TEST_ASSERT_EQUAL(9, worker1_sum);
    TEST_ASSERT_EQUAL(1, ezQueue_GetNumOfElement(&worker1.msg_queue));

    ezTaskWorker_ExecuteTaskNoRTOS();
    TEST_ASSERT_EQUAL(300, worker1_sum);
    TEST_ASSERT_EQUAL(0, ezQueue_GetNumOfElement(&worker1.msg_queue));
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_task_worker);
}


static bool worker1_sum_external(int a, int b)
{

    struct Worker1SumContext contxt;
    bool ret = false;

    contxt.a = a;
    contxt.b = b;

    ezTaskWorker_EnqueueTask(&worker1,
                             worker1_sum_internal,
                             callback1,
                             (void*)&contxt,
                             sizeof(struct Worker1SumContext), 0);
    return true;
}

static bool worker1_sum_internal(void *context, ezTaskWorkerCallbackFunc callback)
{
    bool ret = false;
    int sum = 0;
    struct Worker1SumContext *sum_context = (struct Worker1SumContext*)context; 

    if(sum_context != NULL && callback != NULL)
    {
        sum = sum_context->a + sum_context->b;
        callback(WORKER1_EVENT_SUM_CMPLT, (void*)&sum);
        ret = true;
    }

    return ret;
}

static void callback1(uint8_t event, void *ret_data)
{
    switch ((WORKER1_EVENT)event)
    {
    case WORKER1_EVENT_SUM_CMPLT:
        worker1_sum = *(int*)ret_data;
        break;

    default:
        break;
    }
}


/* End of file */
