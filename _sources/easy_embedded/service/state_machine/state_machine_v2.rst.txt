============================================================
State Machine V2
============================================================

Introduction
============================
This document describes the second version of the State Machine component of the
EzEmbedded Framework.

The State Machine V2 is a table driven, event-driven finite state machine. The whole
behavior of the machine is described by a **transition table**. Each row of the table
says: *in this state, when this event arrives, if the guard allows it, run this action
and go to that state*. The code that runs the machine is generic and never has to be
changed when the behavior changes.

**What this component does:**

- Describes the behavior with constant tables (can be placed in flash)
- Supports entry and exit functions for each state
- Supports guard functions that can block a transition
- Supports actions that run during a transition
- Supports internal transitions (run an action without leaving the state)
- Supports transitions that apply to every state (wildcard)
- Stores events in a queue, so events can be posted from callbacks or interrupts
- Reports what happened to every dispatched event
- Allows data sharing between callbacks via a user-defined context pointer

**What this component does NOT do:**

- Does not support hierarchical (nested) state machines. Use the first version of
  the component if sub-state machines are needed
- Does not support orthogonal regions (parallel states)
- Does not implement history states
- Does not provide automatic code generation from state diagrams
- Does not allocate memory. The user provides the event buffer

**Differences to the first version:**

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * -
     - Version 1
     - Version 2
   * - Behavior
     - Written in code (state functions return the next state)
     - Described by a transition table
   * - Guards
     - Inside the event handler
     - Separate guard function per transition
   * - Event
     - ``uint8_t``
     - ``ezSm_Event_t`` with id (16 bit) and a payload
   * - Result of an event
     - None
     - ``ezSm_Result_t``
   * - Hierarchy
     - Sub-state machines
     - Not supported
   * - Enable option
     - ``ENABLE_EZ_STATE_MACHINE``
     - ``ENABLE_EZ_STATE_MACHINE_V2``

**Use cases:**

- Protocol implementations (communication protocols, handshake sequences)
- Device control logic (power management, mode switching)
- User interface navigation
- Workflow and process control in embedded applications

Component's structure
============================
The component is composed of the following elements:

**ezSm_State_t** - Represents a single state:

- ``id``: Identifier of the state. The ids ``EZ_SM_ANY_STATE`` and ``EZ_SM_STAY`` are reserved
- ``on_entry``: Function executed once when entering the state (optional)
- ``on_exit``: Function executed once when leaving the state (optional)

**ezSm_Transition_t** - Represents one row of the transition table:

- ``current_state``: The state in which the row applies. A state with the id
  ``EZ_SM_ANY_STATE`` makes the row apply in every state
- ``event_id``: The event that triggers the row
- ``guard``: Function that can block the transition (optional)
- ``action``: Function executed during the transition (optional)
- ``next_state``: The state to enter. A state with the id ``EZ_SM_STAY`` makes the row an
  internal transition

**ezSm_Config_t** - Describes the machine. It is constant and can be shared by several
state machine instances:

- ``transitions_table`` and ``transitions_count``: The transition table
- ``initial_state``: The state entered by ``ezSm_Start()``
- ``on_transition``: Called after every state change (optional)
- ``on_unhandled``: Called when no row handles an event (optional)

**ezSm_StateMachine_t** - Represents one instance of the state machine. The fields are
managed by the component and must not be changed by the user:

- ``config``: Pointer to the configuration. ``NULL`` as long as the machine is not initialized
- ``context``: User-defined data pointer passed to every callback
- ``current_state``: The active state
- ``busy``: Set while an event is being processed
- ``event_queue``: Queue holding the pending events

Component's behavior
============================

External Behavior
-----------------
The state machine exposes the following API functions:

- ``ezSm_Init()``: Validates the configuration and creates the event queue
- ``ezSm_Start()``: Enters the initial state and runs its entry function
- ``ezSm_PostEvent()``: Copies an event into the queue. The event is processed later
- ``ezSm_DispatchEvent()``: Processes an event immediately
- ``ezSm_Run()``: Processes all queued events (call periodically)

``ezSm_DispatchEvent()`` returns what happened to the event:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Result
     - Meaning
   * - ``EZ_SM_TRANSITIONED``
     - The state changed
   * - ``EZ_SM_INTERNAL``
     - A row with ``EZ_SM_STAY`` matched. Only the action ran
   * - ``EZ_SM_IGNORED``
     - No row matches this state and event. ``on_unhandled`` was called
   * - ``EZ_SM_GUARD_BLOCKED``
     - Rows exist, but all of their guards returned false
   * - ``EZ_SM_QUEUED``
     - The machine was busy, the event was queued instead
   * - ``EZ_SM_ERROR``
     - Invalid argument, machine not initialized, or the queue is full

Internal Behavior
-----------------
**Initialization.** ``ezSm_Init()`` checks the whole configuration before it changes the
state machine object: the initial state, the table, and the state pointers of every row
must not be ``NULL``. The machine is marked as ready (``config`` is set) only after the
event queue is created. If anything fails, the object stays uninitialized and all other
functions refuse to work with it.

**Matching a row.** ``ezSm_DispatchEvent()`` walks through the table from the first to
the last row and takes the first row that:

1. has the same event id as the event,
2. applies to the current state (same state id, or ``EZ_SM_ANY_STATE``), and
3. has no guard, or its guard returns true.

If the guard of a row returns false, the walk continues with the next row. This allows
several rows for the same state and event, ordered by priority, with an unguarded row at
the end as the default.

.. note::

   Rows are matched in table order. Put the rows of specific states **before** the rows that
   use ``EZ_SM_ANY_STATE``, otherwise the wildcard row hides them.

**Running a transition.** For a row with a next state, the following happens in this order:

1. The ``on_exit`` function of the current state
2. The ``action`` of the row
3. The current state is changed to the next state
4. The ``on_entry`` function of the new state
5. The ``on_transition`` callback of the configuration

The current state is already the new state when ``on_entry`` and ``on_transition`` run.
A transition to the state that is already active is a normal transition: exit and entry
functions run again.

For a row with ``EZ_SM_STAY``, only the action runs. The state, the exit and entry
functions and ``on_transition`` are not touched. The action is optional, a row without
an action simply consumes the event.

**Unhandled events.** If no row matches, ``on_unhandled`` is called and the result is
``EZ_SM_IGNORED``. If at least one row matched but every guard blocked it,
``on_unhandled`` is **not** called and the result is ``EZ_SM_GUARD_BLOCKED``.

**Busy handling.** While an event is processed, the machine is busy. An event dispatched
in this time (for example from an action or from another context) is not processed,
it is put into the queue and ``EZ_SM_QUEUED`` is returned. The test and the set of the busy
flag are protected with the critical section macros. The queued event is processed by the
next ``ezSm_Run()``.

**Running the queue.** ``ezSm_Run()`` takes the events from the queue in FIFO order and
dispatches them one by one. It returns the number of processed events.

- It returns 0 immediately when the machine is busy. Do not call it from a callback
- An entry of the queue that does not have the size of an event is logged and dropped,
  so it cannot block the events behind it

**Flow of a dispatched event:**

.. mermaid::

   flowchart TD
      Start([ezSm_DispatchEvent]) --> Valid{Initialized and<br/>event valid?}
      Valid -->|No| Error([EZ_SM_ERROR])
      Valid -->|Yes| Busy{Busy?}

      Busy -->|Yes| Post[Post event to queue]
      Post --> Queued([EZ_SM_QUEUED])

      Busy -->|No| Next[Take next row]
      Next --> More{Row left?}

      More -->|No| Matched{Any guard<br/>blocked?}
      Matched -->|Yes| Blocked([EZ_SM_GUARD_BLOCKED])
      Matched -->|No| Unhandled["on_unhandled"] --> Ignored([EZ_SM_IGNORED])

      More -->|Yes| Match{Event and state<br/>match?}
      Match -->|No| Next
      Match -->|Yes| Guard{Guard allows?}
      Guard -->|No| Next

      Guard -->|Yes| Stay{Next state is<br/>EZ_SM_STAY?}
      Stay -->|Yes| Action1[action] --> Internal([EZ_SM_INTERNAL])
      Stay -->|No| Exit[on_exit] --> Action2[action] --> Change[Set new state]
      Change --> Entry[on_entry] --> Trans[on_transition] --> Done([EZ_SM_TRANSITIONED])

Data Flow
============================
The following diagram illustrates how events and data flow through the state machine:

.. mermaid::

  flowchart TD
    subgraph Application
      Post["ezSm_PostEvent()"]
      Dispatch["ezSm_DispatchEvent()"]
      Run["ezSm_Run()"]
    end

    subgraph SM["ezSm_StateMachine_t"]
      Queue["event_queue"]
      Current["current_state"]
      Context["context (void*)"]
    end

    subgraph Config["ezSm_Config_t (constant)"]
      Table["transition table"]
      Hooks["on_transition<br/>on_unhandled"]
    end

    subgraph Callbacks["User callbacks"]
      Guard["guard(context, event)"]
      Action["action(context, event)"]
      State["on_entry(context)<br/>on_exit(context)"]
    end

    Post --> Queue
    Run --> Queue
    Queue --> Dispatch
    Dispatch --> Table
    Table --> Guard
    Table --> Action
    Table --> State
    Dispatch --> Current
    Dispatch --> Hooks
    Context --> Guard
    Context --> Action
    Context --> State

**Data exchange between callbacks:**

- Callbacks share data through the ``context`` pointer given to ``ezSm_Init()``
- The pointer is passed unchanged as first parameter to every callback
- Callbacks cast it to the structure type of the application

**Event flow:**

1. Application code calls ``ezSm_PostEvent()``. The event is **copied** into the queue
2. ``ezSm_Run()`` takes the events out of the queue in FIFO order
3. Each event is dispatched: the table is searched and the matching row is executed

.. warning::

   The event is copied, but the data it points to is not. If the payload of an event is a
   pointer (``event.data.data``), the data must stay valid until the event is processed.

Component's data type
============================

Function Pointer Types
----------------------

.. c:type:: ezSm_StateFunction_t

   Entry or exit function of a state.

   :param context: User-defined context pointer

.. c:type:: ezSm_GuardFunction_t

   Function that decides if a transition may be taken.

   :param context: User-defined context pointer
   :param event: The event that is being processed
   :return: true to allow the transition, false to block it

.. c:type:: ezSm_ActionFunction_t

   Function executed during a transition.

   :param context: User-defined context pointer
   :param event: The event that is being processed

Structures
----------

.. c:struct:: ezSm_Event_t

   Represents an event.

   :member id: Event id (``ezSm_EventId_t``, 16 bit)
   :member data: Payload, a union of ``bool data_bool``, ``uint32_t data_size``,
                 ``int32_t data_int``, ``float data_float`` and ``void *data``

.. c:struct:: ezSm_State_t

   Represents a state.

   :member id: State id (``ezSm_StateId_t``, 16 bit)
   :member on_entry: Entry function (can be NULL)
   :member on_exit: Exit function (can be NULL)

.. c:struct:: ezSm_Transition_t

   Represents a row of the transition table.

   :member current_state: State in which the row applies (can be a state with id ``EZ_SM_ANY_STATE``)
   :member next_state: State to enter (can be a state with id ``EZ_SM_STAY``)
   :member guard: Guard function (can be NULL)
   :member action: Action function (can be NULL)
   :member event_id: Event that triggers the row

.. c:struct:: ezSm_Config_t

   Represents the configuration of a state machine.

   :member transitions_table: Pointer to the array of transitions
   :member transitions_count: Number of transitions
   :member initial_state: State entered by ``ezSm_Start()``
   :member on_transition: Callback ``void (*)(void *context, const ezSm_State_t *from, const ezSm_State_t *to, const ezSm_Event_t *event)``,
                          called after a state change (can be NULL)
   :member on_unhandled: Callback ``void (*)(void *context, const ezSm_State_t *state, const ezSm_Event_t *event)``,
                         called when no row handles the event (can be NULL)

.. c:struct:: ezSm_StateMachine_t

   Represents a state machine instance. The members are managed by the component.

   :member config: Pointer to the configuration, NULL until initialized
   :member context: User-defined context pointer
   :member busy: True while an event is processed
   :member event_queue: Queue holding the pending events
   :member current_state: The active state

Enumerations
------------

.. c:enum:: ezSm_Result_t

   Result of ``ezSm_DispatchEvent()``.

   :enumerator EZ_SM_TRANSITIONED: State changed
   :enumerator EZ_SM_INTERNAL: An internal transition was executed
   :enumerator EZ_SM_IGNORED: No row handles the event
   :enumerator EZ_SM_GUARD_BLOCKED: Rows exist, all guards returned false
   :enumerator EZ_SM_QUEUED: Machine was busy, event queued
   :enumerator EZ_SM_ERROR: Invalid argument, not initialized, or queue full

Constants
---------

.. c:macro:: EZ_SM_ANY_STATE

   State id (0xFFFF) that makes a transition row apply to every state.

.. c:macro:: EZ_SM_STAY

   State id (0xFFFE) that makes a transition row an internal transition.

.. c:macro:: EZ_STATE_MACHINE_ENTER_CRITICAL_SECTION

   Macro called before the busy flag or the queue is accessed. Empty by default.
   Define it, together with ``EZ_STATE_MACHINE_EXIT_CRITICAL_SECTION``, when events are
   posted from interrupts or from several tasks.

Usage
============================
The following example implements a simple device with the states *IDLE* and *RUNNING*.
It can be started and stopped, and a *tick* event is only accepted while running.

.. code-block:: c

    #include "ez_state_machine_v2.h"

    typedef enum { ST_IDLE, ST_RUNNING } StateId;
    typedef enum { EV_START = 1, EV_STOP, EV_TICK } EventId;

    typedef struct { uint32_t ticks; } DeviceData;

    static void RunningEntry(void *context) { ((DeviceData *)context)->ticks = 0; }
    static void CountTick(void *context, const ezSm_Event_t *event) { ((DeviceData *)context)->ticks++; }
    static bool IsStartAllowed(void *context, const ezSm_Event_t *event) { return event->data.data_bool; }

    /* States */
    static const ezSm_State_t idle    = {ST_IDLE,    NULL,         NULL};
    static const ezSm_State_t running = {ST_RUNNING, RunningEntry, NULL};
    static const ezSm_State_t stay    = {EZ_SM_STAY, NULL,         NULL};

    /* Transition table: current state, next state, guard, action, event */
    static const ezSm_Transition_t transitions[] = {
        {&idle,    &running, IsStartAllowed, NULL,      EV_START},
        {&running, &idle,    NULL,           NULL,      EV_STOP},
        {&running, &stay,    NULL,           CountTick, EV_TICK},
    };

    static const ezSm_Config_t config = {
        .transitions_table = transitions,
        .transitions_count = sizeof(transitions) / sizeof(transitions[0]),
        .initial_state = &idle,
        .on_transition = NULL,
        .on_unhandled = NULL,
    };

    static ezSm_StateMachine_t sm;      /* zero initialized as a global variable */
    static DeviceData data;
    static uint8_t event_buffer[256];

    void DeviceInit(void)
    {
        if (ezSm_Init(&sm, &config, &data, event_buffer, sizeof(event_buffer)))
        {
            ezSm_Start(&sm);
        }
    }

    void DeviceStart(void)
    {
        ezSm_Event_t event = {.id = EV_START, .data.data_bool = true};
        ezSm_PostEvent(&sm, &event);        /* can be called from an interrupt */
    }

    void DeviceTask(void)
    {
        ezSm_Run(&sm);                      /* call periodically */
    }

Usage rules
-----------
- The ``ezSm_StateMachine_t`` object must be zero initialized (a global or static variable)
  or passed through ``ezSm_Init()`` before any other function is called
- The configuration and all tables must stay valid as long as the state machine is used
- ``ezSm_Run()`` must not be called from a state, guard, action or configuration callback
- Guards should have no side effects, because they can be evaluated for rows that are not taken
- Define the critical section macros if events are posted from interrupts or other tasks.
  ``ezSm_Run()`` and ``ezSm_DispatchEvent()`` themselves must be called from one context
- A malformed table is rejected by ``ezSm_Init()``, but a wrong order of the rows
  (wildcard rows first) cannot be detected and hides the rows behind them

Testing
============================
The unit tests are located in ``tests/service/state_machine/unittest_ez_state_machine_v2.cpp``.
They are written with Catch2 as behavior driven scenarios, and use fff fakes for all callbacks.
