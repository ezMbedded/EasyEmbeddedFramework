============================================================
State Machine
============================================================

Introduction
============================
This document describes the State Machine component of the EzEmbedded Framework.

The State Machine component provides a lightweight, event-driven hierarchical state
machine implementation suitable for embedded systems. It enables developers to model
complex system behavior using well-defined states and transitions.

**What this component does:**

- Provides a structured way to implement finite state machines
- Supports entry, action, and exit functions for each state
- Handles event-driven state transitions via an event queue
- Supports hierarchical state machines (sub-state machines)
- Allows data sharing between states via a user-defined data pointer

**What this component does NOT do:**

- Does not provide automatic code generation from state diagrams
- Does not support orthogonal regions (parallel states)
- Does not implement history states

**Use cases:**

- Protocol implementations (communication protocols, handshake sequences)
- Device control logic (power management, mode switching)
- User interface navigation
- Workflow and process control in embedded applications

Component's structure
============================
The State Machine component is composed of two main structures:

**ezState_t** - Represents a single state with the following elements:

- ``name``: Human-readable state name for debugging
- ``action``: Function executed repeatedly while in this state
- ``enter``: Function executed once when entering this state
- ``exit``: Function executed once when leaving this state
- ``handle_event``: Function to process incoming events
- ``sub_sm``: Pointer to a nested sub-state machine (optional)

**ezStateMachine_t** - Represents the state machine itself:

- ``curr_state``: Pointer to the currently active state
- ``next_state``: Pointer to the next state (used during transitions)
- ``events``: Ring buffer storing pending events
- ``data``: User-defined data pointer shared between states

The component also provides helper macros for defining states:

- ``INIT_STATE(state_name, sub_state_machine)``: Declares and initializes a state
- ``DEFINE_ACTION_FUNCTION(state_name)``: Declares an action function
- ``DEFINE_ENTRY_FUNCTION(state_name)``: Declares an entry function
- ``DEFINE_EXIT_FUNCTION(state_name)``: Declares an exit function
- ``DEFINE_EVENT_HANDLER_FUNCTION(state_name)``: Declares an event handler

Component's behavior
============================

External Behavior
-----------------
The state machine exposes the following API functions:

- ``ezSM_Init()``: Initializes the state machine with an initial state and event buffer
- ``ezSM_Run()``: Gives processing time to the state machine (call periodically)
- ``ezSM_SetEvent()``: Pushes a new event to the state machine's event queue
- ``ezSM_ClearAllEvents()``: Clears all pending events from the queue
- ``ezSM_SetState()``: Forces a state transition to a specific state
- ``ezSM_GetCurrState()``: Returns a pointer to the current state

Internal Behavior
-----------------
The state machine follows this execution model during ``ezSM_Run()``:

1. **Event Processing**: If the current state has an event handler and events are
   pending, the handler is called with the oldest event. The handler may return
   a new state to trigger a transition.

2. **Action Execution**: If no state change occurred from event handling:

   - If a sub-state machine exists, ``ezSM_Run()`` is called recursively on it
   - Otherwise, the state's action function is executed
   - The action function may return a new state to trigger a transition

3. **State Transition**: If a next state is determined:

   - The current state's ``exit`` function is called (if defined)
   - The exit function can override the next state if an error occurs
   - The new state's ``enter`` function is called (if defined)
   - The enter function can redirect to another state (e.g., for error handling)

**State Transition Diagram:**

.. mermaid::

   flowchart TD
      Start([ezSM_Run]) --> EventPending{Event pending?}
      
      EventPending -->|Yes| HandleEvent[handle_event]
      EventPending -->|No| SubSMExists{Sub-SM exists?}
      
      SubSMExists -->|Yes| RunSubSM[ezSM_Run sub]
      SubSMExists -->|No| Action[action]
      
      HandleEvent --> StateChanged{State changed?}
      RunSubSM --> StateChanged
      Action --> StateChanged
      
      StateChanged -->|Yes| Transition["exit -> enter"]
      StateChanged -->|No| Done([Done])
      
      Transition --> Done

Data Flow
============================
The following diagram illustrates how data flows through the state machine:

.. mermaid::

  flowchart TD
    subgraph Application
      SetEvent["ezSM_SetEvent()"]
      Run["ezSM_Run()"]
      GetState["ezSM_GetCurrState()"]
    end

    subgraph SM["ezStateMachine_t"]
      EventQueue["Event Queue\n(RingBuffer)"]
      States["curr_state\nnext_state"]
      Data["data (void*)\n(user-defined struct)"]
    end

    subgraph State["ezState_t"]
      HandleEvent["handle_event(event)\n► Returns next state or NULL"]
      Action["action(sm)\n► Returns next state or NULL"]
      Enter["enter(sm)\n► Initializes state, may redirect"]
      Exit["exit(sm)\n► Cleanup, may redirect on error"]
      SubSM["sub_sm\n(optional nested SM)"]
    end

    SetEvent --> EventQueue
    Run --> States
    States --> GetState

    EventQueue --> HandleEvent
    States --> Action
    Data --> Enter
    Data --> Exit

    Action --> SubSM

**Data exchange between states:**

- States share data through the ``sm->data`` pointer
- This pointer is set during ``ezSM_Init()`` and remains constant
- States can cast it to the appropriate structure type to access shared data

**Event flow:**

1. External code calls ``ezSM_SetEvent()`` to push events to the queue
2. Events are stored in a ring buffer (FIFO order)
3. During ``ezSM_Run()``, events are popped and processed by ``handle_event()``
4. The event handler returns a new state pointer or NULL (no transition)

Component's data type
============================

Function Pointer Types
----------------------

.. c:type:: ezSM_DoFunction

   Function executed repeatedly while the state machine is in a state.

   :param sm: Pointer to the state machine
   :return: Pointer to the next state, or NULL if no state change

.. c:type:: ezSM_EntryFunction

   Function executed once when entering a new state.

   :param sm: Pointer to the state machine
   :return: Pointer to redirect state, or NULL to stay in current state

.. c:type:: ezSM_ExitFunction

   Function executed once when exiting the current state.

   :param sm: Pointer to the state machine
   :return: Pointer to override next state, or NULL to proceed normally

.. c:type:: ezSM_EventHandler

   Function to handle incoming events.

   :param event: The event to be handled (uint8_t)
   :return: Pointer to the next state, or NULL if event doesn't cause transition

Structures
----------

.. c:struct:: ezState

   Represents a single state in the state machine.

   :member name: State name string for debugging (can be NULL)
   :member action: Pointer to the action function (can be NULL)
   :member enter: Pointer to the entry function (can be NULL)
   :member exit: Pointer to the exit function (can be NULL)
   :member handle_event: Pointer to the event handler (can be NULL)
   :member sub_sm: Pointer to a sub-state machine (can be NULL)

.. c:struct:: ezStateMachine

   Represents the state machine instance.

   :member curr_state: Pointer to the currently active state
   :member next_state: Pointer to the next state during transitions
   :member events: Ring buffer for storing pending events
   :member data: User-defined data pointer for inter-state communication

Constants
---------

.. c:macro:: MAX_SUPPORTED_STATE

   Maximum number of supported states (0xFF = 255)
