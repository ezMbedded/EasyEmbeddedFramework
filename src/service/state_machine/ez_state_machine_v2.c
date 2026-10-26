/*****************************************************************************
* Filename:         ez_state_machine_v2.c
* Author:           Hai Nguyen
* Original Date:    25.10.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_state_machine_v2.c
 *  @author Hai Nguyen
 *  @date   25.10.2026
 *  @brief  Table driven state machine with guards, actions and an event queue
 *
 *  @details See ez_state_machine_v2.h for the rules of the transition table
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_state_machine_v2.h"

#if (EZ_STATE_MACHINE_V2 == 1)
#include <string.h>
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_STATE_MACHINE_LOGGING_LEVEL      /**< logging level */
#define MOD_NAME    "ez_state_machine"                  /**< module name */
#include "ez_logging.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static bool ezSm_IsConfigValid(const ezSm_Config_t *config);
static bool ezSm_IsReady(const ezSm_StateMachine_t *sm);
static void ezSm_RunEntryFunction(ezSm_StateMachine_t *sm, const ezSm_State_t *state);
static void ezSm_RunExitFunction(ezSm_StateMachine_t *sm, const ezSm_State_t *state);


/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezSm_Init(ezSm_StateMachine_t *sm,
    const ezSm_Config_t *config,
    void *context,
    uint8_t *event_buff, size_t event_buff_size)
{
    if (sm == NULL
        || event_buff == NULL
        || event_buff_size == 0
        || ezSm_IsConfigValid(config) == false)
    {
        EZERROR("Invalid parameters");
        return false;
    }

    /* Stays "not ready" until everything succeeded */
    sm->config = NULL;
    if(ezQueue_CreateQueue(&sm->event_queue, event_buff, (uint32_t)event_buff_size) == ezFAIL)
    {
        EZERROR("Failed to create event queue");
        return false;
    }

    sm->context = context;
    sm->busy = false;
    sm->current_state = config->initial_state;
    sm->config = config;
    return true;
}

void ezSm_Start(ezSm_StateMachine_t *sm)
{
    if(ezSm_IsReady(sm) == false)
    {
        return;
    }

    sm->current_state = sm->config->initial_state;
    ezSm_RunEntryFunction(sm, sm->current_state);
}

bool ezSm_PostEvent(ezSm_StateMachine_t *sm, const ezSm_Event_t *event)
{
    bool status = false;
    if(ezSm_IsReady(sm) == false || event == NULL)
    {
        EZERROR("Invalid parameters");
        return status;
    }

    EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION();
    if(ezQueue_Push(&sm->event_queue, (void*)event, sizeof(ezSm_Event_t)) == ezSUCCESS)
    {
        status = true;
    }
    EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION();

    return status;
}


ezSm_Result_t ezSm_DispatchEvent(ezSm_StateMachine_t *sm, const ezSm_Event_t *event)
{
    bool was_busy = false;
    if(ezSm_IsReady(sm) == false || event == NULL)
    {
        EZERROR("Invalid parameters");
        return EZ_SM_ERROR;
    }

    EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION();
    was_busy = sm->busy;
    sm->busy = true;
    EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION();

    if(was_busy)
    {
        /* The caller that owns the busy flag keeps it, we only queue */
        if(ezSm_PostEvent(sm, event))
        {
            return EZ_SM_QUEUED;
        }
        EZERROR("Failed to queue event");
        return EZ_SM_ERROR;
    }

    const ezSm_Config_t *config = sm->config;
    const ezSm_State_t *current_state = sm->current_state;
    bool guard_blocked = false;

    for(size_t i = 0; i < config->transitions_count; i++)
    {
        const ezSm_Transition_t *transition = &config->transitions_table[i];

        /* Skip because this transition does not support this event */
        if(transition->event_id != event->id)
        {
            continue;
        }

        /* Skip because current state does not have this transition
         * or transition does not apply to every state
         */
        if(current_state->id != transition->current_state->id
            && transition->current_state->id != EZ_SM_ANY_STATE)
        {
            continue;
        }

        /* Transition is blocked by guard */
        if(transition->guard != NULL
            && transition->guard(sm->context, event) == false)
        {
            guard_blocked = true;
            continue;
        }

        ezSm_Result_t result = EZ_SM_ERROR;
        /* Internal transition */
        if(transition->next_state->id == EZ_SM_STAY)
        {
            if(transition->action != NULL)
            {
                transition->action(sm->context, event);
            }
            result = EZ_SM_INTERNAL;
        }
        else
        {
            const ezSm_State_t *from = sm->current_state;
            ezSm_RunExitFunction(sm, from);
            if(transition->action != NULL)
            {
                transition->action(sm->context, event);
            }
            sm->current_state = transition->next_state;
            ezSm_RunEntryFunction(sm, sm->current_state);
            if(config->on_transition != NULL)
            {
                config->on_transition(sm->context,
                    from,
                    sm->current_state,
                    event);
            }
            result = EZ_SM_TRANSITIONED;
        }
        sm->busy = false;
        return result;
    }

    if(guard_blocked == false && config->on_unhandled != NULL)
    {
        config->on_unhandled(sm->context, sm->current_state, event);
    }
    sm->busy = false;

    return guard_blocked ? EZ_SM_GUARD_BLOCKED : EZ_SM_IGNORED;
}


size_t ezSm_Run(ezSm_StateMachine_t *sm)
{
    size_t num_processed_event = 0;

    /* Never run from inside a callback, the events would be re-queued forever */
    if(ezSm_IsReady(sm) == false || sm->busy)
    {
        return num_processed_event;
    }

    for(;;)
    {
        ezSm_Event_t event;
        void *front = NULL;
        size_t event_size = sizeof(ezSm_Event_t);
        bool got_event = false;
        bool valid_event = false;

        EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION();
        if(ezQueue_GetFront(&sm->event_queue, &front, &event_size) == ezSUCCESS)
        {
            got_event = true;
            if(event_size == sizeof(ezSm_Event_t))
            {
                memcpy(&event, front, sizeof(ezSm_Event_t));
                valid_event = true;
            }
            /* A malformed entry is dropped as well, otherwise it blocks the queue */
            ezQueue_PopFront(&sm->event_queue);
        }
        EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION();

        if(got_event == false)
        {
            break;
        }

        if(valid_event == false)
        {
            EZERROR("Dropped event with unexpected size");
            continue;
        }

        if(ezSm_DispatchEvent(sm, &event) == EZ_SM_ERROR)
        {
            EZERROR("Event %u failed to dispatch", (unsigned int)event.id);
        }
        num_processed_event++;
    }
    return num_processed_event;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static bool ezSm_IsConfigValid(const ezSm_Config_t *config)
{
    if(config == NULL
        || config->initial_state == NULL
        || config->transitions_table == NULL
        || config->transitions_count == 0)
    {
        return false;
    }

    for(size_t i = 0; i < config->transitions_count; i++)
    {
        const ezSm_Transition_t *transition = &config->transitions_table[i];
        if(transition->current_state == NULL || transition->next_state == NULL)
        {
            return false;
        }
    }
    return true;
}

static bool ezSm_IsReady(const ezSm_StateMachine_t *sm)
{
    return sm != NULL && sm->config != NULL;
}

static void ezSm_RunEntryFunction(ezSm_StateMachine_t *sm, const ezSm_State_t *state)
{
    if(sm == NULL || state == NULL)
    {
        EZDEBUG("Invalid parameters");
        return;
    }

    if(state->on_entry != NULL)
    {
        state->on_entry(sm->context);
    }
}

static void ezSm_RunExitFunction(ezSm_StateMachine_t *sm, const ezSm_State_t *state)
{
    if(sm == NULL || state == NULL)
    {
        EZDEBUG("Invalid parameters");
        return;
    }

    if(state->on_exit != NULL)
    {
        state->on_exit(sm->context);
    }
}


#endif /* EZ_STATE_MACHINE_V2 == 1 */
/* End of file*/
