/*****************************************************************************
* Filename:         unittest_ez_state_machine.c
* Author:           Hai Nguyen
* Original Date:    02.05.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_state_machine.c
 *  @author Hai Nguyen
 *  @date   02.05.2024
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
#include "ez_state_machine.h"

TEST_GROUP(ez_state_machine);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define TEST_BUFF_SIZE      128
#define EVENT_RX            0x00
#define EVENT_TX            0x01

/******************************************************************************
* Module Typedefs
*******************************************************************************/
typedef struct
{
    uint8_t buff[TEST_BUFF_SIZE];
    uint8_t num_written_bytes;
}TestSmData;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/

static ezStateMachine_t test_statemachine;
static uint8_t EventBuff[TEST_BUFF_SIZE] = {0};
static TestSmData test_sm_data;

INIT_STATE(Receive, NULL);
INIT_STATE(Transmit, NULL);

ezState_t *state_table[] = {&Receive, &Transmit, NULL};

uint8_t receive_enter_count = 0;
uint8_t receive_exit_count = 0;
uint8_t receive_action_count = 0;
uint8_t receive_handle_count = 0;

uint8_t transmit_enter_count = 0;
uint8_t transmit_exit_count = 0;
uint8_t transmit_action_count = 0;
uint8_t transmit_handle_count = 0;

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);

DEFINE_ACTION_FUNCTION(Receive)
{
    receive_action_count++;
    return NULL;
}

DEFINE_ENTRY_FUNCTION(Receive)
{
    receive_enter_count++;
    return NULL;
}

DEFINE_EXIT_FUNCTION(Receive)
{
    receive_exit_count++;
    return NULL;
}

DEFINE_EVENT_HANDLER_FUNCTION(Receive)
{
    ezState_t *state = NULL;
    switch(event)
    {
        case EVENT_RX:
            /* Handle it here */
            break;

        case EVENT_TX:
            state = &Transmit;
            break;

        default:
            break;
    }
    receive_handle_count++;
    return state;
}


DEFINE_ACTION_FUNCTION(Transmit)
{
    transmit_action_count++;
    ezState_t *state = &Receive;
    return NULL;
}

DEFINE_ENTRY_FUNCTION(Transmit)
{
    transmit_enter_count++;
    return NULL;
}

DEFINE_EXIT_FUNCTION(Transmit)
{
    transmit_exit_count++;
    return NULL;
}

DEFINE_EVENT_HANDLER_FUNCTION(Transmit)
{
    transmit_handle_count;
    return NULL;
}



/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_state_machine)
{
    ezSM_Init(&test_statemachine, &Receive, EventBuff, TEST_BUFF_SIZE, &test_sm_data);
}


TEST_TEAR_DOWN(ez_state_machine)
{
    memset(&test_statemachine, 0, sizeof(ezStateMachine_t));

    receive_enter_count = 0;
    receive_exit_count = 0;
    receive_action_count = 0;
    receive_handle_count = 0;

    transmit_enter_count = 0;
    transmit_exit_count = 0;
    transmit_action_count = 0;
    transmit_handle_count = 0;
}


TEST_GROUP_RUNNER(ez_state_machine)
{
    RUN_TEST_CASE(ez_state_machine, InitStateMachineFail);
    RUN_TEST_CASE(ez_state_machine, InitState);
    RUN_TEST_CASE(ez_state_machine, AddEvent);
    RUN_TEST_CASE(ez_state_machine, TransitionByTXEvent);
    RUN_TEST_CASE(ez_state_machine, StayAtReceiveState);
    RUN_TEST_CASE(ez_state_machine, ActionInTrasmitGoBackToReceive);
}


TEST(ez_state_machine, InitStateMachineFail)
{
    bool ret = false;

    ret = ezSM_Init(NULL, &Receive, EventBuff, TEST_BUFF_SIZE, &test_sm_data);
    TEST_ASSERT_EQUAL(false, ret);

    ezSM_Init(&test_statemachine, NULL, EventBuff, TEST_BUFF_SIZE, &test_sm_data);
    TEST_ASSERT_EQUAL(false, ret);

    ezSM_Init(&test_statemachine, &Receive, EventBuff, TEST_BUFF_SIZE, &test_sm_data);
    TEST_ASSERT_EQUAL(false, ret);

    ezSM_Init(&test_statemachine, &Receive, NULL, TEST_BUFF_SIZE, &test_sm_data);
    TEST_ASSERT_EQUAL(false, ret);

    ezSM_Init(&test_statemachine, &Receive, EventBuff, 0, &test_sm_data);
    TEST_ASSERT_EQUAL(false, ret);
}

TEST(ez_state_machine, InitState)
{
    ezState_t *curr_state = ezSM_GetCurrState(&test_statemachine);
    TEST_ASSERT_EQUAL_MEMORY(curr_state, &Receive, sizeof(ezState_t));
    TEST_ASSERT_EQUAL(1, receive_enter_count);
}

TEST(ez_state_machine, AddEvent)
{
    bool ret = false;
    ret = ezSM_SetEvent(&test_statemachine, EVENT_RX);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_SetEvent(&test_statemachine, EVENT_RX);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_SetEvent(&test_statemachine, EVENT_RX);
    TEST_ASSERT_EQUAL(true, ret);
}

TEST(ez_state_machine, TransitionByTXEvent)
{
    bool ret = false;
    ret = ezSM_SetEvent(&test_statemachine, EVENT_TX);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);

    TEST_ASSERT_EQUAL(1, transmit_enter_count);
    TEST_ASSERT_EQUAL(1, receive_handle_count);
    TEST_ASSERT_EQUAL(1, receive_exit_count);
}

TEST(ez_state_machine, StayAtReceiveState)
{
    bool ret = false;
    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);

    TEST_ASSERT_EQUAL(1, receive_enter_count);
    TEST_ASSERT_EQUAL(3, receive_action_count);
    TEST_ASSERT_EQUAL(0, receive_exit_count);
}

TEST(ez_state_machine, ActionInTrasmitGoBackToReceive)
{
    bool ret = false;
    ret = ezSM_SetEvent(&test_statemachine, EVENT_TX);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);

    ret = ezSM_Run(&test_statemachine);
    TEST_ASSERT_EQUAL(true, ret);
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_state_machine);
}


/* End of file */
