/*****************************************************************************
* Filename:         ez_state_machine.h
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

/** @file   ez_state_machine.h
 *  @author Hai Nguyen
 *  @date   02.05.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

#ifndef _EZ_STATE_MACHINE_H
#define _EZ_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_STATE_MACHINE == 1)

#include <stdint.h>
#include <stdbool.h>
#include "ez_ring_buffer.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define MAX_SUPPORTED_STATE     0xFF   /**< Maximum supported states */

/* Macro to define an action function */
#define DEFINE_ACTION_FUNCTION(state_name) \
    static ezState_t* state_name##Action(ezStateMachine_t *sm)

/* Macro to define an entry function */
#define DEFINE_ENTRY_FUNCTION(state_name) \
    static ezState_t* state_name##Entry(ezStateMachine_t *sm)

/* Macro to define an exit function */
#define DEFINE_EXIT_FUNCTION(state_name) \
    static ezState_t* state_name##Exit(ezStateMachine_t *sm)

/* Macro to define an event handler function */
#define DEFINE_EVENT_HANDLER_FUNCTION(state_name) \
    static ezState_t* state_name##HandleEvent(uint8_t event)

/* Macro to initialize a state */
#define INIT_STATE(state_name, sub_state_machine)           \
    static ezState_t* state_name##Action(ezStateMachine_t *sm); \
    static ezState_t* state_name##Entry(ezStateMachine_t *sm);      \
    static ezState_t* state_name##Exit(ezStateMachine_t *sm);       \
    static ezState_t* state_name##HandleEvent(uint8_t event); \
    static ezState_t state_name = {                           \
        .name = #state_name,                                \
        .action = state_name##Action,                       \
        .enter = state_name##Entry,                         \
        .exit = state_name##Exit,                           \
        .handle_event = state_name##HandleEvent,            \
        .sub_sm = sub_state_machine                         \
    }                                                       \


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

typedef struct ezState ezState_t;
typedef struct ezStateMachine ezStateMachine_t;

/** @brief Define a pointer to a function executed when the state machine in a state
 *
 *  @param sm: (IN)Pointer to the state machine
 * 
 *  @return Next State or NULL (no state change)
 */
typedef ezState_t* (*ezSM_DoFunction)(ezStateMachine_t *sm);


/** @brief  Define a pointer to a function executed when the state machine enter
 *          new state
 *
 *  @param  sm: (IN)Pointer to the state machine
 * 
 *  @return None
 */
typedef ezState_t* (*ezSM_EntryFunction)(ezStateMachine_t *sm);


/** @brief  Define a pointer to a function executed when the state machine exits
 *          the current state
 *
 *  @param  sm: (IN)Pointer to the state machine
 * 
 *  @return None
 */
typedef ezState_t* (*ezSM_ExitFunction)(ezStateMachine_t *sm);


/** @brief  Define a pointer to a function handling the incomming event
 *  @param  event: (IN)event to be handled
 * 
 *  @return Next State or NULL (no state change)
 */
typedef ezState_t* (*ezSM_EventHandler)(uint8_t event);


/** @brief Structure holding data of a state
 *  
 */
struct ezState
{
    char *name;                                     /**< State name. Can be NULL */
    ezSM_DoFunction action;                           /**< Pointer to do function. Can be NULL*/
    ezSM_EntryFunction enter;                         /**< Pointer to entry function. Can be NULL */
    ezSM_ExitFunction exit;                           /**< Pointer to exit function. Can be NULL */
    ezSM_EventHandler handle_event;                   /**< Pointer to event handler. Can be NULL */
    ezStateMachine_t *sub_sm;                         /**< Pointer to sub state machine. Can be NULL*/
};


/** @brief Structure holding data of a state machine
 *  
 */
struct ezStateMachine
{
    ezState_t *curr_state;    /**< Pointer to current state */
    ezState_t *next_state;    /**< Pointer to the Next state */
    RingBuffer events;      /**< Ring buffer containing the events */
    void *data;             /**< Pointer to data exchanged between state. Can be NULL */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/
/******************************************************************************
* Function : sm_Init
*//** 
* @Description:
*
* This function initializes the ring buffer
* 
* @param    sm: (IN)Pointer to the state machine which must be initialized
* @param    init_state: (IN)Pointer to initial state
* @param    event_buff: (IN)Pointer to buffer which stores the event
* @param    event_buff_size: (IN)Size of the buffer
* @param    data: (IN)Pointer to structure holding data exchanged between states. Can be NULL.
*
* @return   true if success, else false
*
*******************************************************************************/
bool ezSM_Init(ezStateMachine_t *sm,
               ezState_t *init_state,
               uint8_t *event_buff,
               uint32_t event_buff_size,
               void *data);

/******************************************************************************
* 
* Function : sm_Run
*//** 
* @Description:
*
* This function gives processing time to the state machine
* 
* @param    sm: (IN)Pointer to the state machine
* @return   true if success, else false
*
*******************************************************************************/
bool ezSM_Run(ezStateMachine_t *sm);


/******************************************************************************
* 
* Function : sm_SetEvent
*//** 
* @Description:
*
* This function set a new event to the state machine
* 
* @param    sm: (IN)Pointer to the state machine
* @param    event: (IN)Event
* @return   true if success, else false
*
*******************************************************************************/
bool ezSM_SetEvent(ezStateMachine_t *sm, uint8_t event);


/******************************************************************************
* 
* Function : sm_ClearAllEvents
*//** 
* @Description:
*
* This function clears all events in the event queue
* 
* @param    sm: (IN)Pointer to the state machine
* @return   true if success, else false
*
*******************************************************************************/
bool ezSM_ClearAllEvents(ezStateMachine_t *sm);


/******************************************************************************
* 
* Function : sm_SetState
*//** 
* @Description:
*
* This function set the next states of the state machine
* 
* @param    sm: (IN)Pointer to the state machine
* @param    new_state: (IN)state will be set
* @return   true if success, else false
*
*******************************************************************************/
bool ezSM_SetState(ezStateMachine_t *sm, ezState_t *new_state);


/******************************************************************************
* 
* Function : sm_GetCurrState
*//** 
* @Description:
*
* This function returns the current state
* 
* @param    sm: (IN)Pointer to the state machine
* @return   Current state
*
*******************************************************************************/
ezState_t* ezSM_GetCurrState(ezStateMachine_t *sm);

#ifdef __cplusplus
}
#endif

#endif /* EZ_STATE_MACHINE == 1 */
#endif /* _EZ_STATE_MACHINE_H */


/* End of file */
