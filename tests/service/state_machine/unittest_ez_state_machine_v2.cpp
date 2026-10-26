/*****************************************************************************
* Filename:         unittest_ez_state_machine_v2.cpp
* Author:           Hai Nguyen
* Original Date:    26.10.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_state_machine_v2.cpp
 *  @author Hai Nguyen
 *  @date   26.10.2026
 *  @brief  Unit tests of the table driven state machine (version 2)
 *
 *  @details The tests are written as BDD scenarios (SCENARIO/GIVEN/WHEN/THEN).
 *  All user callbacks (entry/exit functions, guards, actions, config callbacks)
 *  are fff fakes. The event queue is the real ez_queue component: it is a
 *  separate, already tested utility and the state machine only works with it
 *  through a thin layer, so mocking it would not add confidence.
 *
 *  The transition table used by every scenario:
 *
 *  | State   | Event        | Guard | Action    | Next state          |
 *  |---------|--------------|-------|-----------|---------------------|
 *  | IDLE    | EV_START     | -     | act_start | RUNNING             |
 *  | RUNNING | EV_STOP      | -     | act_stop  | IDLE                |
 *  | RUNNING | EV_TICK      | -     | act_tick  | STAY (internal)     |
 *  | RUNNING | EV_NOOP      | -     | -         | STAY (internal)     |
 *  | IDLE    | EV_GUARDED   | guard | act_guard | RUNNING             |
 *  | IDLE    | EV_FALLTHRU  | guard | act_guard | RUNNING             |
 *  | IDLE    | EV_FALLTHRU  | -     | -         | ERROR               |
 *  | RUNNING | EV_REPEAT    | -     | -         | RUNNING (self)      |
 *  | ANY     | EV_RESET     | -     | act_reset | IDLE                |
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ez_state_machine_v2.h"
#include "fff.h"

#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;

/* State entry and exit functions */
FAKE_VOID_FUNC(entry_idle, void *);
FAKE_VOID_FUNC(exit_idle, void *);
FAKE_VOID_FUNC(entry_running, void *);
FAKE_VOID_FUNC(exit_running, void *);
FAKE_VOID_FUNC(entry_error, void *);

/* Transition actions and guard */
FAKE_VOID_FUNC(act_start, void *, const ezSm_Event_t *);
FAKE_VOID_FUNC(act_stop, void *, const ezSm_Event_t *);
FAKE_VOID_FUNC(act_tick, void *, const ezSm_Event_t *);
FAKE_VOID_FUNC(act_guarded, void *, const ezSm_Event_t *);
FAKE_VOID_FUNC(act_reset, void *, const ezSm_Event_t *);
FAKE_VALUE_FUNC(bool, guard_fake, void *, const ezSm_Event_t *);

/* Configuration callbacks */
FAKE_VOID_FUNC(on_transition_fake, void *, const ezSm_State_t *, const ezSm_State_t *, const ezSm_Event_t *);
FAKE_VOID_FUNC(on_unhandled_fake, void *, const ezSm_State_t *, const ezSm_Event_t *);

#define EVENT_BUFF_SIZE     512U    /**< Event buffer, enough for several events */
#define MAX_POST_ATTEMPTS   1000U   /**< Upper limit to fill the event queue */
#define FAKE_HISTORY(fake)  reinterpret_cast<fff_function_t>(fake)


/******************************************************************************
* Module Typedefs
*******************************************************************************/
enum StateId : uint16_t
{
    ST_IDLE = 0,
    ST_RUNNING,
    ST_ERROR,
};

enum EventId : uint16_t
{
    EV_START = 1,
    EV_STOP,
    EV_TICK,
    EV_NOOP,
    EV_GUARDED,
    EV_FALLTHRU,
    EV_REPEAT,
    EV_RESET,
    EV_UNKNOWN = 99,
};


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static const ezSm_State_t state_idle    = {ST_IDLE, entry_idle, exit_idle};
static const ezSm_State_t state_running = {ST_RUNNING, entry_running, exit_running};
static const ezSm_State_t state_error   = {ST_ERROR, entry_error, nullptr};
static const ezSm_State_t state_any     = {EZ_SM_ANY_STATE, nullptr, nullptr};
static const ezSm_State_t state_stay    = {EZ_SM_STAY, nullptr, nullptr};

static const ezSm_Transition_t transitions[] = {
    /* current       next            guard       action        event */
    {&state_idle,    &state_running, nullptr,    act_start,    EV_START},
    {&state_running, &state_idle,    nullptr,    act_stop,     EV_STOP},
    {&state_running, &state_stay,    nullptr,    act_tick,     EV_TICK},
    {&state_running, &state_stay,    nullptr,    nullptr,      EV_NOOP},
    {&state_idle,    &state_running, guard_fake, act_guarded,  EV_GUARDED},
    {&state_idle,    &state_running, guard_fake, act_guarded,  EV_FALLTHRU},
    {&state_idle,    &state_error,   nullptr,    nullptr,      EV_FALLTHRU},
    {&state_running, &state_running, nullptr,    nullptr,      EV_REPEAT},
    {&state_any,     &state_idle,    nullptr,    act_reset,    EV_RESET},
};

static ezSm_StateMachine_t *g_sm = nullptr;     /**< State machine used by re-entrant fakes */
static ezSm_Result_t g_reentrant_result;        /**< Result of a dispatch made inside a callback */
static size_t g_run_in_callback_result;         /**< Result of ezSm_Run called inside a callback */


/******************************************************************************
* Function Definitions
*******************************************************************************/
static ezSm_Event_t MakeEvent(uint16_t id);
static void DispatchStopFromAction(void *context, const ezSm_Event_t *event);
static void RunFromEntry(void *context);

/**
 * Common setup of all scenarios: every fake is reset and the state machine
 * object is zero initialized, like a global variable would be.
 */
class StateMachineFixture
{
public:
    StateMachineFixture()
    {
        RESET_FAKE(entry_idle);
        RESET_FAKE(exit_idle);
        RESET_FAKE(entry_running);
        RESET_FAKE(exit_running);
        RESET_FAKE(entry_error);
        RESET_FAKE(act_start);
        RESET_FAKE(act_stop);
        RESET_FAKE(act_tick);
        RESET_FAKE(act_guarded);
        RESET_FAKE(act_reset);
        RESET_FAKE(guard_fake);
        RESET_FAKE(on_transition_fake);
        RESET_FAKE(on_unhandled_fake);
        FFF_RESET_HISTORY();

        memset(&sm, 0, sizeof(sm));
        memset(event_buff, 0, sizeof(event_buff));
        g_sm = &sm;
        g_reentrant_result = EZ_SM_ERROR;
        g_run_in_callback_result = 0;

        config.transitions_table = transitions;
        config.transitions_count = sizeof(transitions) / sizeof(transitions[0]);
        config.initial_state = &state_idle;
        config.on_transition = on_transition_fake;
        config.on_unhandled = on_unhandled_fake;
    }

    ~StateMachineFixture()
    {
        g_sm = nullptr;
    }

protected:
    bool InitStateMachine()
    {
        return ezSm_Init(&sm, &config, &context, event_buff, sizeof(event_buff));
    }

    /** Init and Start, then clear the call history so tests only see their own calls */
    void InitAndStartStateMachine()
    {
        REQUIRE(InitStateMachine());
        ezSm_Start(&sm);
        REQUIRE(sm.current_state == &state_idle);
        RESET_FAKE(entry_idle);
        FFF_RESET_HISTORY();
    }

    /** Move the already started state machine from IDLE to RUNNING */
    void MoveToRunning()
    {
        ezSm_Event_t start = MakeEvent(EV_START);
        REQUIRE(ezSm_DispatchEvent(&sm, &start) == EZ_SM_TRANSITIONED);
        REQUIRE(sm.current_state == &state_running);
        RESET_FAKE(exit_idle);
        RESET_FAKE(entry_running);
        RESET_FAKE(act_start);
        RESET_FAKE(on_transition_fake);
        FFF_RESET_HISTORY();
    }

    ezSm_StateMachine_t sm;
    ezSm_Config_t config;
    uint8_t event_buff[EVENT_BUFF_SIZE];
    int context = 0;    /**< Only its address is used, to check the context pass-through */
};


/******************************************************************************
* External functions
*******************************************************************************/
SCENARIO_METHOD(StateMachineFixture, "State machine initialization", "[service][state_machine_v2][init]")
{
    GIVEN("Invalid arguments")
    {
        WHEN("ezSm_Init is called with a NULL state machine")
        {
            THEN("It fails")
            {
                CHECK_FALSE(ezSm_Init(nullptr, &config, &context, event_buff, sizeof(event_buff)));
            }
        }

        WHEN("ezSm_Init is called with a NULL configuration")
        {
            THEN("It fails and the state machine stays uninitialized")
            {
                CHECK_FALSE(ezSm_Init(&sm, nullptr, &context, event_buff, sizeof(event_buff)));
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("ezSm_Init is called with a NULL event buffer")
        {
            THEN("It fails and the state machine stays uninitialized")
            {
                CHECK_FALSE(ezSm_Init(&sm, &config, &context, nullptr, sizeof(event_buff)));
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("ezSm_Init is called with an event buffer of size 0")
        {
            THEN("It fails and the state machine stays uninitialized")
            {
                CHECK_FALSE(ezSm_Init(&sm, &config, &context, event_buff, 0));
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("The event buffer is too small to create the queue")
        {
            THEN("It fails and the state machine stays uninitialized")
            {
                CHECK_FALSE(ezSm_Init(&sm, &config, &context, event_buff, 1));
                CHECK(sm.config == nullptr);
            }
        }
    }

    GIVEN("A configuration with invalid content")
    {
        WHEN("The initial state is NULL")
        {
            config.initial_state = nullptr;

            THEN("ezSm_Init fails")
            {
                CHECK_FALSE(InitStateMachine());
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("The transition table is NULL")
        {
            config.transitions_table = nullptr;

            THEN("ezSm_Init fails")
            {
                CHECK_FALSE(InitStateMachine());
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("The transition table is empty")
        {
            config.transitions_count = 0;

            THEN("ezSm_Init fails")
            {
                CHECK_FALSE(InitStateMachine());
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("A transition has no current state")
        {
            static const ezSm_Transition_t bad[] = {
                {nullptr, &state_running, nullptr, nullptr, EV_START},
            };
            config.transitions_table = bad;
            config.transitions_count = 1;

            THEN("ezSm_Init fails, a dispatch can never dereference it")
            {
                CHECK_FALSE(InitStateMachine());
                CHECK(sm.config == nullptr);
            }
        }

        WHEN("A transition has no next state")
        {
            static const ezSm_Transition_t bad[] = {
                {&state_idle, nullptr, nullptr, nullptr, EV_START},
            };
            config.transitions_table = bad;
            config.transitions_count = 1;

            THEN("ezSm_Init fails, a dispatch can never dereference it")
            {
                CHECK_FALSE(InitStateMachine());
                CHECK(sm.config == nullptr);
            }
        }
    }

    GIVEN("A valid configuration")
    {
        WHEN("ezSm_Init is called")
        {
            bool result = InitStateMachine();

            THEN("It succeeds and the state machine is ready in the initial state")
            {
                CHECK(result);
                CHECK(sm.config == &config);
                CHECK(sm.context == &context);
                CHECK(sm.current_state == &state_idle);
                CHECK_FALSE(sm.busy);
            }

            THEN("No state function is called before ezSm_Start")
            {
                CHECK(entry_idle_fake.call_count == 0);
                CHECK(on_transition_fake_fake.call_count == 0);
            }
        }

        WHEN("The context is NULL")
        {
            THEN("ezSm_Init still succeeds")
            {
                CHECK(ezSm_Init(&sm, &config, nullptr, event_buff, sizeof(event_buff)));
                CHECK(sm.context == nullptr);
            }
        }

        WHEN("The on_transition and on_unhandled callbacks are NULL")
        {
            config.on_transition = nullptr;
            config.on_unhandled = nullptr;

            THEN("ezSm_Init still succeeds")
            {
                CHECK(InitStateMachine());
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "State machine start", "[service][state_machine_v2][start]")
{
    GIVEN("A NULL or uninitialized state machine")
    {
        WHEN("ezSm_Start is called with NULL")
        {
            THEN("Nothing happens and nothing crashes")
            {
                ezSm_Start(nullptr);
                CHECK(entry_idle_fake.call_count == 0);
            }
        }

        WHEN("ezSm_Start is called before ezSm_Init")
        {
            ezSm_Start(&sm);

            THEN("No entry function is called")
            {
                CHECK(entry_idle_fake.call_count == 0);
                CHECK(sm.current_state == nullptr);
            }
        }
    }

    GIVEN("An initialized state machine")
    {
        REQUIRE(InitStateMachine());

        WHEN("ezSm_Start is called")
        {
            ezSm_Start(&sm);

            THEN("The entry function of the initial state runs once with the context")
            {
                CHECK(entry_idle_fake.call_count == 1);
                CHECK(entry_idle_fake.arg0_val == &context);
                CHECK(sm.current_state == &state_idle);
            }
        }

        WHEN("The initial state has no entry function")
        {
            static const ezSm_State_t plain_state = {ST_ERROR, nullptr, nullptr};
            config.initial_state = &plain_state;
            REQUIRE(InitStateMachine());

            THEN("ezSm_Start does not crash and selects the state")
            {
                ezSm_Start(&sm);
                CHECK(sm.current_state == &plain_state);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Posting events", "[service][state_machine_v2][post]")
{
    GIVEN("Invalid arguments")
    {
        ezSm_Event_t event = MakeEvent(EV_START);

        WHEN("The state machine is NULL")
        {
            THEN("ezSm_PostEvent fails")
            {
                CHECK_FALSE(ezSm_PostEvent(nullptr, &event));
            }
        }

        WHEN("The state machine was never initialized")
        {
            THEN("ezSm_PostEvent fails")
            {
                CHECK_FALSE(ezSm_PostEvent(&sm, &event));
            }
        }

        WHEN("The event is NULL")
        {
            REQUIRE(InitStateMachine());

            THEN("ezSm_PostEvent fails")
            {
                CHECK_FALSE(ezSm_PostEvent(&sm, nullptr));
            }
        }
    }

    GIVEN("An initialized state machine")
    {
        InitAndStartStateMachine();
        ezSm_Event_t event = MakeEvent(EV_START);

        WHEN("An event is posted")
        {
            bool result = ezSm_PostEvent(&sm, &event);

            THEN("It is queued and not processed yet")
            {
                CHECK(result);
                CHECK(act_start_fake.call_count == 0);
                CHECK(sm.current_state == &state_idle);
            }
        }

        WHEN("More events are posted than the queue can hold")
        {
            size_t posted = 0;
            bool last_result = true;
            while (last_result && posted < MAX_POST_ATTEMPTS)
            {
                last_result = ezSm_PostEvent(&sm, &event);
                if (last_result)
                {
                    posted++;
                }
            }

            THEN("ezSm_PostEvent eventually reports a full queue")
            {
                CHECK(posted > 0);
                CHECK(posted < MAX_POST_ATTEMPTS);
                CHECK_FALSE(last_result);
            }

            THEN("Running the machine processes exactly the accepted events")
            {
                CHECK(ezSm_Run(&sm) == posted);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events - argument checks", "[service][state_machine_v2][dispatch]")
{
    ezSm_Event_t event = MakeEvent(EV_START);

    GIVEN("A NULL or uninitialized state machine")
    {
        WHEN("ezSm_DispatchEvent is called with a NULL state machine")
        {
            THEN("It returns EZ_SM_ERROR")
            {
                CHECK(ezSm_DispatchEvent(nullptr, &event) == EZ_SM_ERROR);
            }
        }

        WHEN("ezSm_DispatchEvent is called before ezSm_Init")
        {
            THEN("It returns EZ_SM_ERROR and calls nothing")
            {
                CHECK(ezSm_DispatchEvent(&sm, &event) == EZ_SM_ERROR);
                CHECK(act_start_fake.call_count == 0);
            }
        }
    }

    GIVEN("An initialized state machine")
    {
        InitAndStartStateMachine();

        WHEN("ezSm_DispatchEvent is called with a NULL event")
        {
            THEN("It returns EZ_SM_ERROR and the state is untouched")
            {
                CHECK(ezSm_DispatchEvent(&sm, nullptr) == EZ_SM_ERROR);
                CHECK(sm.current_state == &state_idle);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events - state transitions", "[service][state_machine_v2][dispatch]")
{
    GIVEN("A started state machine in the IDLE state")
    {
        InitAndStartStateMachine();
        ezSm_Event_t event = MakeEvent(EV_START);

        WHEN("A matching event is dispatched")
        {
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &event);

            THEN("The state machine moves to the next state")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_running);
                CHECK_FALSE(sm.busy);
            }

            THEN("Exit, action, entry and on_transition run in this order")
            {
                REQUIRE(fff.call_history_idx == 4);
                CHECK(fff.call_history[0] == FAKE_HISTORY(exit_idle));
                CHECK(fff.call_history[1] == FAKE_HISTORY(act_start));
                CHECK(fff.call_history[2] == FAKE_HISTORY(entry_running));
                CHECK(fff.call_history[3] == FAKE_HISTORY(on_transition_fake));
            }

            THEN("Every callback receives the context")
            {
                CHECK(exit_idle_fake.arg0_val == &context);
                CHECK(act_start_fake.arg0_val == &context);
                CHECK(entry_running_fake.arg0_val == &context);
                CHECK(on_transition_fake_fake.arg0_val == &context);
            }

            THEN("The action receives the dispatched event")
            {
                CHECK(act_start_fake.arg1_val == &event);
            }

            THEN("on_transition reports the old and the new state")
            {
                CHECK(on_transition_fake_fake.arg1_val == &state_idle);
                CHECK(on_transition_fake_fake.arg2_val == &state_running);
                CHECK(on_transition_fake_fake.arg3_val == &event);
            }

            THEN("The unhandled callback is not called")
            {
                CHECK(on_unhandled_fake_fake.call_count == 0);
            }
        }

        WHEN("The on_transition callback is not configured")
        {
            config.on_transition = nullptr;
            REQUIRE(InitStateMachine());
            ezSm_Start(&sm);

            THEN("The transition still works")
            {
                CHECK(ezSm_DispatchEvent(&sm, &event) == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_running);
            }
        }

        WHEN("The event is not handled in the current state")
        {
            ezSm_Event_t stop = MakeEvent(EV_STOP);
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &stop);

            THEN("It is ignored and the state does not change")
            {
                CHECK(result == EZ_SM_IGNORED);
                CHECK(sm.current_state == &state_idle);
                CHECK(act_stop_fake.call_count == 0);
                CHECK(exit_idle_fake.call_count == 0);
            }

            THEN("on_unhandled reports the state and the event")
            {
                REQUIRE(on_unhandled_fake_fake.call_count == 1);
                CHECK(on_unhandled_fake_fake.arg0_val == &context);
                CHECK(on_unhandled_fake_fake.arg1_val == &state_idle);
                CHECK(on_unhandled_fake_fake.arg2_val == &stop);
            }

            THEN("The busy flag is released")
            {
                CHECK_FALSE(sm.busy);
            }
        }

        WHEN("The event id is not in the table at all")
        {
            ezSm_Event_t unknown = MakeEvent(EV_UNKNOWN);

            THEN("It is ignored")
            {
                CHECK(ezSm_DispatchEvent(&sm, &unknown) == EZ_SM_IGNORED);
                CHECK(on_unhandled_fake_fake.call_count == 1);
            }
        }

        WHEN("The event is not handled and on_unhandled is not configured")
        {
            config.on_unhandled = nullptr;
            REQUIRE(InitStateMachine());
            ezSm_Start(&sm);
            ezSm_Event_t unknown = MakeEvent(EV_UNKNOWN);

            THEN("It is still ignored without a crash")
            {
                CHECK(ezSm_DispatchEvent(&sm, &unknown) == EZ_SM_IGNORED);
            }
        }
    }

    GIVEN("A state machine in the RUNNING state")
    {
        InitAndStartStateMachine();
        MoveToRunning();

        WHEN("An event leads back to the IDLE state")
        {
            ezSm_Event_t stop = MakeEvent(EV_STOP);
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &stop);

            THEN("The RUNNING exit function runs, then the IDLE entry function")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_idle);
                REQUIRE(fff.call_history_idx == 4);
                CHECK(fff.call_history[0] == FAKE_HISTORY(exit_running));
                CHECK(fff.call_history[1] == FAKE_HISTORY(act_stop));
                CHECK(fff.call_history[2] == FAKE_HISTORY(entry_idle));
                CHECK(fff.call_history[3] == FAKE_HISTORY(on_transition_fake));
            }
        }

        WHEN("A transition targets the state that is already active")
        {
            ezSm_Event_t repeat = MakeEvent(EV_REPEAT);
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &repeat);

            THEN("It is an external transition: exit and entry run again")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_running);
                REQUIRE(fff.call_history_idx == 3);
                CHECK(fff.call_history[0] == FAKE_HISTORY(exit_running));
                CHECK(fff.call_history[1] == FAKE_HISTORY(entry_running));
                CHECK(fff.call_history[2] == FAKE_HISTORY(on_transition_fake));
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events - internal transitions", "[service][state_machine_v2][dispatch]")
{
    GIVEN("A state machine in the RUNNING state")
    {
        InitAndStartStateMachine();
        MoveToRunning();

        WHEN("An event with an internal transition and an action is dispatched")
        {
            ezSm_Event_t tick = MakeEvent(EV_TICK);
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &tick);

            THEN("Only the action runs")
            {
                CHECK(result == EZ_SM_INTERNAL);
                CHECK(sm.current_state == &state_running);
                CHECK(act_tick_fake.call_count == 1);
                CHECK(act_tick_fake.arg0_val == &context);
                CHECK(act_tick_fake.arg1_val == &tick);
                CHECK(exit_running_fake.call_count == 0);
                CHECK(entry_running_fake.call_count == 0);
                CHECK(on_transition_fake_fake.call_count == 0);
                CHECK_FALSE(sm.busy);
            }
        }

        WHEN("An event with an internal transition and no action is dispatched")
        {
            ezSm_Event_t noop = MakeEvent(EV_NOOP);
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &noop);

            THEN("The event is consumed and reported as internal, not as an error")
            {
                CHECK(result == EZ_SM_INTERNAL);
                CHECK(sm.current_state == &state_running);
                CHECK(fff.call_history_idx == 0);
                CHECK(on_unhandled_fake_fake.call_count == 0);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events - guards", "[service][state_machine_v2][dispatch]")
{
    GIVEN("A state machine in the IDLE state and a guarded transition")
    {
        InitAndStartStateMachine();
        ezSm_Event_t guarded = MakeEvent(EV_GUARDED);

        WHEN("The guard allows the transition")
        {
            guard_fake_fake.return_val = true;
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &guarded);

            THEN("The transition is taken")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_running);
                CHECK(act_guarded_fake.call_count == 1);
            }

            THEN("The guard receives the context and the event")
            {
                REQUIRE(guard_fake_fake.call_count == 1);
                CHECK(guard_fake_fake.arg0_val == &context);
                CHECK(guard_fake_fake.arg1_val == &guarded);
            }

            THEN("The guard is evaluated before any state function")
            {
                REQUIRE(fff.call_history_idx >= 2);
                CHECK(fff.call_history[0] == FAKE_HISTORY(guard_fake));
                CHECK(fff.call_history[1] == FAKE_HISTORY(exit_idle));
            }
        }

        WHEN("The guard blocks the transition")
        {
            guard_fake_fake.return_val = false;
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &guarded);

            THEN("Nothing runs and the state does not change")
            {
                CHECK(result == EZ_SM_GUARD_BLOCKED);
                CHECK(sm.current_state == &state_idle);
                CHECK(act_guarded_fake.call_count == 0);
                CHECK(exit_idle_fake.call_count == 0);
                CHECK(entry_running_fake.call_count == 0);
                CHECK(on_transition_fake_fake.call_count == 0);
                CHECK_FALSE(sm.busy);
            }

            THEN("The event is not reported as unhandled")
            {
                CHECK(on_unhandled_fake_fake.call_count == 0);
            }
        }
    }

    GIVEN("Two rows for the same state and event: a guarded one, then an unguarded one")
    {
        InitAndStartStateMachine();
        ezSm_Event_t fallthru = MakeEvent(EV_FALLTHRU);

        WHEN("The guard of the first row allows the transition")
        {
            guard_fake_fake.return_val = true;
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &fallthru);

            THEN("The first row wins")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_running);
                CHECK(entry_error_fake.call_count == 0);
            }
        }

        WHEN("The guard of the first row blocks the transition")
        {
            guard_fake_fake.return_val = false;
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &fallthru);

            THEN("The next matching row is evaluated and taken")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_error);
                CHECK(entry_error_fake.call_count == 1);
                CHECK(act_guarded_fake.call_count == 0);
                CHECK(on_unhandled_fake_fake.call_count == 0);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events - transitions from any state", "[service][state_machine_v2][dispatch]")
{
    GIVEN("A state machine in the IDLE state")
    {
        InitAndStartStateMachine();
        ezSm_Event_t reset = MakeEvent(EV_RESET);

        WHEN("The wildcard event is dispatched")
        {
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &reset);

            THEN("The wildcard row applies and the real state is exited")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_idle);
                CHECK(act_reset_fake.call_count == 1);
                CHECK(exit_idle_fake.call_count == 1);
                CHECK(entry_idle_fake.call_count == 1);
            }

            THEN("on_transition reports the real state, not the wildcard")
            {
                REQUIRE(on_transition_fake_fake.call_count == 1);
                CHECK(on_transition_fake_fake.arg1_val == &state_idle);
                CHECK(on_transition_fake_fake.arg2_val == &state_idle);
            }
        }
    }

    GIVEN("A state machine in the RUNNING state")
    {
        InitAndStartStateMachine();
        MoveToRunning();
        ezSm_Event_t reset = MakeEvent(EV_RESET);

        WHEN("The wildcard event is dispatched")
        {
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &reset);

            THEN("The exit function of the RUNNING state runs, not the wildcard one")
            {
                CHECK(result == EZ_SM_TRANSITIONED);
                CHECK(sm.current_state == &state_idle);
                CHECK(exit_running_fake.call_count == 1);
                CHECK(entry_idle_fake.call_count == 1);
            }

            THEN("on_transition reports RUNNING as the source state")
            {
                REQUIRE(on_transition_fake_fake.call_count == 1);
                CHECK(on_transition_fake_fake.arg1_val == &state_running);
                CHECK(on_transition_fake_fake.arg2_val == &state_idle);
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Dispatching events while busy", "[service][state_machine_v2][dispatch]")
{
    GIVEN("A started state machine")
    {
        InitAndStartStateMachine();
        ezSm_Event_t start = MakeEvent(EV_START);

        WHEN("ezSm_DispatchEvent is called while the machine is busy")
        {
            sm.busy = true;
            ezSm_Result_t result = ezSm_DispatchEvent(&sm, &start);

            THEN("The event is queued and not processed")
            {
                CHECK(result == EZ_SM_QUEUED);
                CHECK(sm.current_state == &state_idle);
                CHECK(act_start_fake.call_count == 0);
            }

            THEN("The busy flag is left to its owner")
            {
                CHECK(sm.busy);
            }

            AND_WHEN("The machine becomes idle and runs")
            {
                sm.busy = false;
                size_t processed = ezSm_Run(&sm);

                THEN("The queued event is processed")
                {
                    CHECK(processed == 1);
                    CHECK(sm.current_state == &state_running);
                }
            }
        }

        WHEN("ezSm_DispatchEvent is called while busy and the queue is full")
        {
            size_t posted = 0;
            while (ezSm_PostEvent(&sm, &start) && posted < MAX_POST_ATTEMPTS)
            {
                posted++;
            }
            REQUIRE(posted < MAX_POST_ATTEMPTS);
            sm.busy = true;

            THEN("It returns EZ_SM_ERROR")
            {
                CHECK(ezSm_DispatchEvent(&sm, &start) == EZ_SM_ERROR);
            }
        }

        WHEN("An action dispatches another event")
        {
            act_start_fake.custom_fake = DispatchStopFromAction;
            ezSm_Result_t outer = ezSm_DispatchEvent(&sm, &start);

            THEN("The inner event is queued, the running transition completes first")
            {
                CHECK(outer == EZ_SM_TRANSITIONED);
                CHECK(g_reentrant_result == EZ_SM_QUEUED);
                CHECK(sm.current_state == &state_running);
                CHECK_FALSE(sm.busy);
            }

            AND_WHEN("The queue is run")
            {
                size_t processed = ezSm_Run(&sm);

                THEN("The queued event is processed afterwards")
                {
                    CHECK(processed == 1);
                    CHECK(sm.current_state == &state_idle);
                    CHECK(act_stop_fake.call_count == 1);
                }
            }
        }
    }
}


SCENARIO_METHOD(StateMachineFixture, "Running the event queue", "[service][state_machine_v2][run]")
{
    GIVEN("A NULL or uninitialized state machine")
    {
        WHEN("ezSm_Run is called with NULL")
        {
            THEN("It processes nothing")
            {
                CHECK(ezSm_Run(nullptr) == 0);
            }
        }

        WHEN("ezSm_Run is called before ezSm_Init")
        {
            THEN("It processes nothing")
            {
                CHECK(ezSm_Run(&sm) == 0);
            }
        }
    }

    GIVEN("A started state machine with an empty queue")
    {
        InitAndStartStateMachine();

        WHEN("ezSm_Run is called")
        {
            THEN("It returns 0 and nothing is called")
            {
                CHECK(ezSm_Run(&sm) == 0);
                CHECK(fff.call_history_idx == 0);
            }
        }
    }

    GIVEN("A started state machine with several queued events")
    {
        InitAndStartStateMachine();
        ezSm_Event_t start = MakeEvent(EV_START);
        ezSm_Event_t tick = MakeEvent(EV_TICK);
        ezSm_Event_t stop = MakeEvent(EV_STOP);
        REQUIRE(ezSm_PostEvent(&sm, &start));
        REQUIRE(ezSm_PostEvent(&sm, &tick));
        REQUIRE(ezSm_PostEvent(&sm, &stop));

        WHEN("ezSm_Run is called")
        {
            size_t processed = ezSm_Run(&sm);

            THEN("All events are processed")
            {
                CHECK(processed == 3);
                CHECK(sm.current_state == &state_idle);
                CHECK_FALSE(sm.busy);
            }

            THEN("The events are processed in the order they were posted")
            {
                REQUIRE(fff.call_history_idx == 9);
                CHECK(fff.call_history[0] == FAKE_HISTORY(exit_idle));
                CHECK(fff.call_history[1] == FAKE_HISTORY(act_start));
                CHECK(fff.call_history[2] == FAKE_HISTORY(entry_running));
                CHECK(fff.call_history[3] == FAKE_HISTORY(on_transition_fake));
                CHECK(fff.call_history[4] == FAKE_HISTORY(act_tick));
                CHECK(fff.call_history[5] == FAKE_HISTORY(exit_running));
                CHECK(fff.call_history[6] == FAKE_HISTORY(act_stop));
                CHECK(fff.call_history[7] == FAKE_HISTORY(entry_idle));
                CHECK(fff.call_history[8] == FAKE_HISTORY(on_transition_fake));
            }

            AND_WHEN("ezSm_Run is called again")
            {
                THEN("The queue is empty, nothing is processed twice")
                {
                    CHECK(ezSm_Run(&sm) == 0);
                    CHECK(act_start_fake.call_count == 1);
                }
            }
        }

        WHEN("ezSm_Run is called while the machine is busy")
        {
            sm.busy = true;

            THEN("It returns 0 and leaves the events in the queue")
            {
                CHECK(ezSm_Run(&sm) == 0);
                CHECK(act_start_fake.call_count == 0);

                sm.busy = false;
                CHECK(ezSm_Run(&sm) == 3);
            }
        }
    }

    GIVEN("A state machine whose entry function calls ezSm_Run")
    {
        entry_running_fake.custom_fake = RunFromEntry;
        InitAndStartStateMachine();
        ezSm_Event_t start = MakeEvent(EV_START);
        ezSm_Event_t tick = MakeEvent(EV_TICK);
        REQUIRE(ezSm_PostEvent(&sm, &tick));

        WHEN("The state is entered through a dispatch")
        {
            REQUIRE(ezSm_DispatchEvent(&sm, &start) == EZ_SM_TRANSITIONED);

            THEN("The nested ezSm_Run refuses to run and returns 0")
            {
                CHECK(g_run_in_callback_result == 0);
                CHECK(act_tick_fake.call_count == 0);
            }

            THEN("The queued event is still available afterwards")
            {
                CHECK(ezSm_Run(&sm) == 1);
                CHECK(act_tick_fake.call_count == 1);
            }
        }
    }

    GIVEN("A queue holding an entry that is not an event, followed by a valid event")
    {
        InitAndStartStateMachine();
        uint8_t garbage = 0xA5;
        ezSm_Event_t start = MakeEvent(EV_START);
        REQUIRE(ezQueue_Push(&sm.event_queue, &garbage, sizeof(garbage)) == ezSUCCESS);
        REQUIRE(ezSm_PostEvent(&sm, &start));

        WHEN("ezSm_Run is called")
        {
            size_t processed = ezSm_Run(&sm);

            THEN("The malformed entry is dropped and does not block the valid event")
            {
                CHECK(processed == 1);
                CHECK(sm.current_state == &state_running);
                CHECK(act_start_fake.call_count == 1);
                CHECK(ezSm_Run(&sm) == 0);
            }
        }
    }
}


/******************************************************************************
* Local functions
*******************************************************************************/
static ezSm_Event_t MakeEvent(uint16_t id)
{
    ezSm_Event_t event;
    memset(&event, 0, sizeof(event));
    event.id = id;
    return event;
}

/** Custom action that dispatches a second event, like user code might do */
static void DispatchStopFromAction(void *context, const ezSm_Event_t *event)
{
    (void)context;
    (void)event;
    ezSm_Event_t stop = MakeEvent(EV_STOP);
    g_reentrant_result = ezSm_DispatchEvent(g_sm, &stop);
}

/** Custom entry function that (wrongly) runs the event queue from a callback */
static void RunFromEntry(void *context)
{
    (void)context;
    g_run_in_callback_result = ezSm_Run(g_sm);
}

/* End of file */
