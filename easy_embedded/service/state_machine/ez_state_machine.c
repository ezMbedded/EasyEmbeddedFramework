/*****************************************************************************
* Filename:         ez_state_machine.c
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

/** @file   ez_state_machine.c
 *  @author Hai Nguyen
 *  @date   02.05.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_state_machine.h"

#if (EZ_STATE_MACHINE == 1)

#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_state_machine"       /**< module name */
#include "ez_logging.h"

/*the rest of include go here*/

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
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezSM_Init(ezStateMachine_t *sm,
             ezState_t *init_state,
             uint8_t *event_buff,
             uint32_t event_buff_size,
             void *data)
{
    EZTRACE("sm_Init()");
    bool success = false;

    if((NULL != sm)
        && (NULL != init_state)
        && (NULL != event_buff)
        && (event_buff_size > 0))
    {
        success = ezRingBuffer_Init(&sm->events, event_buff, event_buff_size);
        if(false == success)
        {
            EZERROR("  init ring buffer error");
        }

        sm->curr_state = init_state;
        success &= ((sm->curr_state != NULL) ? true : false);
        
        if(false == success)
        {
            EZERROR("  Cannot get init state");
        }
 
        sm->next_state = sm->curr_state;
        sm->data = data;

        if(NULL != sm->curr_state->enter)
        {
            sm->curr_state->enter(sm);
        }
    }
    else
    {
        EZERROR("  Input arguments error");
    }

    return success;
}


bool ezSM_Run(ezStateMachine_t *sm)
{
    bool success = false;
    uint8_t event = 0xFF;

    if(NULL != sm)
    {
        success = true;

        /* Clear next_state variable which is set by the previous state */
        if(NULL != sm->next_state )
        {
            EZTRACE("sm_Run(state name = %s)", sm->curr_state->name);
            sm->next_state = NULL;
        }

        /* If we have the handle_event function, we handle the event */
        if(NULL != sm->curr_state->handle_event)
        {
            if(ezRingBuffer_Pop(&sm->events, &event, 1) == 1)
            {
                EZDEBUG("  New event = %d, calling handling function...", event);
                sm->next_state = sm->curr_state->handle_event(event);
            }
        }

        /* No state change or event is not handled */
        if(NULL == sm->next_state)
        {
            /* Have sub state machine */
            if(NULL != sm->curr_state->sub_sm)
            {
                success &= ezSM_Run(sm->curr_state->sub_sm);
                if(false == success)
                {
                    EZERROR("  Run sub statemachine failed");
                }
            }
            /* Have action function */
            else if(NULL != sm->curr_state->action)
            {
                sm->next_state = sm->curr_state->action(sm);
            }
            /* No action, may stuck here forever */
            else
            {
                /* No action, state machine may not advance */
                EZWARNING("  State machine may not advance");
            }
        }

        /* Detect state change*/
        if(NULL != sm->next_state)
        {
            ezState_t* tmp_state = NULL;
            EZDEBUG("  State changed! Next state = %s", sm->next_state->name);

            if(NULL != sm->curr_state->exit)
            {
                EZDEBUG("  Calling exit function...");
                tmp_state = sm->curr_state->exit(sm);
            }

            if(tmp_state != NULL && tmp_state != sm->next_state)
            {
                /* Exit state report a different state from sm->next_state
                 * meaning that there is an issue with the operation within exit function
                 * So we enter the tmp state instead of next_state
                 */
                sm->curr_state = tmp_state;
            }
            else
            {
                sm->curr_state = sm->next_state;
            }

            if(NULL != sm->curr_state->enter)
            {
                EZDEBUG("  Calling enter function...");
                tmp_state = sm->curr_state->enter(sm);
                while(tmp_state != NULL && tmp_state != sm->curr_state)
                {
                    /* Enter state report a different state from sm->curr_state
                     * meaning that there is an issue with the operation within entry function
                     * So we enter the tmp_state instead of the above curr_state
                     */
                    sm->curr_state = tmp_state;
                    tmp_state = sm->curr_state->enter(sm);
                }
            }
        }
    }

    return success;
}


bool ezSM_SetEvent(ezStateMachine_t *sm, uint8_t event)
{
    EZTRACE("sm_SetEvent(envent = %d)", event);
    bool success = false;

    if((NULL != sm)
       && (ezRingBuffer_Push(&sm->events, &event, 1) == 1))
    {
        success = true;
    }

    return success;
}


bool ezSM_ClearAllEvents(ezStateMachine_t *sm)
{
    bool success = false;

    if(NULL != sm)
    {
         ezRingBuffer_Reset(&sm->events);
         success = true;
    }

    return success;
}


bool ezSM_SetState(ezStateMachine_t *sm, ezState_t *new_state)
{
    bool success = false;

    if(NULL != sm && NULL != new_state)
    {
        sm->curr_state = new_state;
        if(NULL != sm->curr_state->enter)
        {
            EZTRACE("  Calling enter function...");
            sm->curr_state->enter(sm);
        }
        success = true;
    }

    return success;
}


ezState_t* ezSM_GetCurrState(ezStateMachine_t *sm)
{
    if(NULL != sm)
    {
        return sm->curr_state;
    }
    return NULL;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

#endif /* EZ_STATE_MACHINE == 1 */
/* End of file*/
