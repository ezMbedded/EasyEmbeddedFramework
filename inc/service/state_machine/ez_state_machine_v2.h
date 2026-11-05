/*****************************************************************************
* Filename:         ez_state_machine_v2.h
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

/** @file   ez_state_machine_v2.h
 *  @author Hai Nguyen
 *  @date   25.10.2026
 *  @brief  Table driven state machine with guards, actions and an event queue
 *
 *  @details The state machine is described by a table of transitions. Each row
 *  names the current state, the triggering event, an optional guard, an
 *  optional action and the next state.
 *
 *  Rules for the transition table:
 *  - Rows are matched in table order. Rows that use EZ_SM_ANY_STATE must be
 *    placed after the rows of specific states, otherwise they shadow them.
 *  - If the guard of a matching row returns false, the following rows are
 *    still evaluated.
 *  - A row whose next state is EZ_SM_STAY runs only the action (internal
 *    transition). No exit/entry function is called.
 *  - A transition to the same state runs the exit and the entry function.
 *  - Order of a transition: exit function, action, entry function,
 *    on_transition callback. The current state is already the new state when
 *    the entry function and on_transition callback run.
 *
 *  Usage rules:
 *  - The state machine object must be zero initialized or passed through
 *    ezSm_Init() before any other call.
 *  - ezSm_Run() must not be called from a state, guard, action or config
 *    callback.
 *  - The pointer stored in event data must outlive the queued event.
 */

#ifndef EZ_STATE_MACHINE_V2_H
#define EZ_STATE_MACHINE_V2_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_STATE_MACHINE_V2 == 1)

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ez_queue.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/** @brief Enter a critical section around queue access and the busy flag. Empty by default, define it in ez_target_config.h */
#ifndef EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION
#define EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION()
/** @brief Exit the critical section. Empty by default */
#define EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION()
#endif

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef uint16_t ezSm_StateId_t; /**< Type to define the state id */
typedef uint16_t ezSm_EventId_t; /**< Type to define the event id */

#define EZ_SM_ANY_STATE ((ezSm_StateId_t)0xFFFF)    /**< match every state */
#define EZ_SM_STAY      ((ezSm_StateId_t)0xFFFE)    /**< Internal transition, run action only */

/** @brief Structure to define an event in the state machine */
typedef struct
{
    ezSm_EventId_t id;  /**< Event id */
    union
    {
        bool data_bool;
        uint32_t data_size;
        int32_t data_int;
        float data_float;
        void *data;
    }data;              /**< Optional event payload */
}ezSm_Event_t;

typedef void (*ezSm_StateFunction_t)(void *context); /**< Type to define the entry/exit function pointer */
typedef bool (*ezSm_GuardFunction_t)(void *context, const ezSm_Event_t *event); /**< Type to define the guard function pointer. Return false to block the transition */
typedef void (*ezSm_ActionFunction_t)(void *context, const ezSm_Event_t *event); /**< Type to define the action function pointer */

/** @brief Structure to define a state in the state machine */
typedef struct
{
    ezSm_StateId_t id;              /**< State id */
    ezSm_StateFunction_t on_entry;  /**< Pointer to the entry function. Can be NULL */
    ezSm_StateFunction_t on_exit;   /**< Pointer to the exit function. Can be NULL */
}ezSm_State_t;

/** @brief Structure to define a transition in the state machine */
typedef struct
{
    const ezSm_State_t *current_state;  /**< State in which the transition applies, or a state with id EZ_SM_ANY_STATE */
    const ezSm_State_t *next_state;     /**< Next state, or a state with id EZ_SM_STAY for an internal transition */
    ezSm_GuardFunction_t guard;         /**< Pointer to the guard function. Can be NULL */
    ezSm_ActionFunction_t action;       /**< Pointer to the action function. Can be NULL */
    ezSm_EventId_t event_id;            /**< Event id that triggers the transition */
}ezSm_Transition_t;

/** @brief Structure to define the configuration of the state machine */
typedef struct
{
    const ezSm_Transition_t *transitions_table; /**< Pointer to the array of transitions */
    size_t transitions_count; /**< Number of transitions in the table */
    const ezSm_State_t *initial_state; /**< Pointer to the initial state */
    void (*on_transition)(void *context, const ezSm_State_t *from, const ezSm_State_t *to, const ezSm_Event_t *event); /**< Called after a state change, once the entry function of the new state ran. Can be NULL */
    void (*on_unhandled)(void *context, const ezSm_State_t *state, const ezSm_Event_t *event); /**< Called when no transition matches the event. Not called when a guard blocked it. Can be NULL */
}ezSm_Config_t;


/** @brief Structure to define the state machine */
typedef struct
{
    const ezSm_Config_t *config; /**< Pointer to the state machine configuration. NULL until initialized */
    void *context; /**< Pointer to context data exchanged between states. Can be NULL */

    bool busy; /**< Flag indicating if the state machine is busy processing an event */
    ezQueue event_queue; /**< Queue to hold events */
    const ezSm_State_t *current_state; /**< Pointer to the current state */
}ezSm_StateMachine_t;


/** @brief Result of dispatching an event */
typedef enum
{
    EZ_SM_TRANSITIONED,     /**< state changed                                 */
    EZ_SM_INTERNAL,         /**< matched an EZ_SM_STAY row                     */
    EZ_SM_IGNORED,          /**< no row for this (state, event)                */
    EZ_SM_GUARD_BLOCKED,    /**< rows exist but every guard returned false     */
    EZ_SM_QUEUED,           /**< called while busy; event was queued instead   */
    EZ_SM_ERROR,            /**< error occurred (e.g. invalid argument)        */
}ezSm_Result_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/


/*****************************************************************************
* Function : ezSm_Init
*//**
* @brief Initializes the state machine with the provided configuration, context and event buffer
*
* @details The configuration is validated: the configuration pointer, the
* initial state, the transition table and the current and next state pointers
* of every transition must not be NULL, and the transition count must not be 0.
* The event queue is created in event_buff, so the buffer determines how many
* events can be queued. The state machine stays not initialized if this
* function fails.
*
* @param[in]    sm: Pointer to the state machine
* @param[in]    config: Pointer to the state machine configuration. Not copied,
*               must outlive the state machine
* @param[in]    context: Pointer to the context data given to every callback. Can be NULL
* @param[in]    event_buff: Memory buffer given to the event queue. Owned by the
*               caller, must outlive the state machine
* @param[in]    event_buff_size: Size of the event buffer in bytes
* @return       bool
*
* @retval true      State machine initialized
* @retval false     A pointer is NULL, event_buff_size is 0, the configuration is
*                   invalid, or the event queue could not be created
*
* @pre None
* @post The current state is the initial state, but its entry function is not
* run until ezSm_Start() is called
* @isr_safe{no}
* @blocking{no}
* @see ezSm_Start
*
* \b Example
* @code
* static uint8_t event_buff[128];
* ezSm_StateMachine_t sm;
* if (ezSm_Init(&sm, &config, NULL, event_buff, sizeof(event_buff)) == false)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool ezSm_Init(ezSm_StateMachine_t *sm,
    const ezSm_Config_t *config,
    void *context,
    uint8_t *event_buff, size_t event_buff_size);


/*****************************************************************************
* Function : ezSm_Start
*//**
* @brief Enters the initial state and runs its entry function
*
* @details Does nothing if the state machine is NULL or not initialized. The
* on_transition callback is not called.
*
* @param[in]    sm: Pointer to the state machine
*
* @pre ezSm_Init() succeeded
* @post The current state is the initial state
* @warning Call it once after ezSm_Init(). The entry function of the initial
* state runs on every call.
* @isr_safe{no}
* @blocking{no}
* @see ezSm_Init
*
* \b Example
* @code
* ezSm_Start(&sm);
* @endcode
*
*****************************************************************************/
void ezSm_Start(ezSm_StateMachine_t *sm);


/*****************************************************************************
* Function : ezSm_PostEvent
*//**
* @brief Copies the event into the event queue
*
* @details The event is processed later by ezSm_Run(). The queue operation is
* enclosed by EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION() and
* EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION(), which are empty unless defined by
* the target configuration.
*
* @param[in]    sm: Pointer to the state machine
* @param[in]    event: Pointer to the event, copied into the queue. A pointer
*               stored in the event data is copied, not the data it points to
* @return       bool
*
* @retval true      Event queued
* @retval false     sm is NULL or not initialized, event is NULL, or the queue is full
*
* @pre ezSm_Init() succeeded
* @post The event is appended to the queue
* @warning With the default (empty) critical section macros this function is
* not protected against concurrent access, for example from an ISR.
* @isr_safe{no}
* @blocking{no}
* @see ezSm_Run, ezSm_DispatchEvent
*
* \b Example
* @code
* ezSm_Event_t event = { .id = EVENT_BUTTON };
* if (ezSm_PostEvent(&sm, &event) == false)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool ezSm_PostEvent(ezSm_StateMachine_t *sm, const ezSm_Event_t *event);


/*****************************************************************************
* Function : ezSm_DispatchEvent
*//**
* @brief Processes the event immediately
*
* @details The transition table is searched in order. If the state machine is
* already busy (called from a callback or from another context), the event is
* queued instead and EZ_SM_QUEUED is returned. The user callbacks (guard,
* action, entry, exit, on_transition, on_unhandled) run in the caller context.
*
* @param[in]    sm: Pointer to the state machine
* @param[in]    event: Pointer to the event
* @return       ezSm_Result_t
*
* @retval EZ_SM_TRANSITIONED    State changed
* @retval EZ_SM_INTERNAL        A row with next state EZ_SM_STAY matched, its action ran
* @retval EZ_SM_IGNORED         No row matches the current state and event.
*                               on_unhandled is called if set
* @retval EZ_SM_GUARD_BLOCKED   Matching rows exist but every guard returned
*                               false. on_unhandled is not called
* @retval EZ_SM_QUEUED          State machine was busy, event was queued
* @retval EZ_SM_ERROR           sm is NULL or not initialized, event is NULL, or
*                               the state machine was busy and the queue is full
*
* @pre ezSm_Init() and ezSm_Start() were called
* @post The current state may have changed
* @warning The busy flag is not reset on EZ_SM_ERROR returned for a busy state machine;
* the caller that owns the flag keeps it.
* @isr_safe{no}
* @blocking{no}
* @see ezSm_PostEvent, ezSm_Run
*
* \b Example
* @code
* ezSm_Event_t event = { .id = EVENT_BUTTON };
* if (ezSm_DispatchEvent(&sm, &event) == EZ_SM_ERROR)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSm_Result_t ezSm_DispatchEvent(ezSm_StateMachine_t *sm, const ezSm_Event_t *event);


/*****************************************************************************
* Function : ezSm_Run
*//**
* @brief Processes all queued events
*
* @details Pops the queued events in order and dispatches each one with
* ezSm_DispatchEvent() until the queue is empty. Events posted by callbacks
* during the run are processed in the same call. An entry in the queue with an
* unexpected size is dropped and not counted. A failed dispatch is logged and
* still counted.
*
* @param[in]    sm: Pointer to the state machine
* @return       size_t Number of processed events
*
* @retval 0     Nothing processed, or sm is NULL or not initialized, or the
*               state machine is busy (called from a callback)
*
* @pre ezSm_Init() and ezSm_Start() were called
* @post The event queue is empty
* @warning Must not be called from a state, guard, action or config callback.
* @isr_safe{no}
* @blocking{no}
* @see ezSm_PostEvent
*
* \b Example
* @code
* size_t num = ezSm_Run(&sm);
* @endcode
*
*****************************************************************************/
size_t ezSm_Run(ezSm_StateMachine_t *sm);

#ifdef __cplusplus
}
#endif

#endif /* EZ_STATE_MACHINE_V2 == 1 */
#endif /* EZ_STATE_MACHINE_V2_H */


/* End of file */
