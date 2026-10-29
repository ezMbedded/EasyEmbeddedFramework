============================================================
Event Bus
============================================================

Introduction
============================
The **Event Bus** component acts as a central communication hub, facilitating the implementation
of the **Publisher-Subscriber** design pattern. It allows different parts of the system to
communicate asynchronously or synchronously without being tightly coupled.

**Key Features:**

*   **Decoupling:** Publishers do not need to know about subscribers.
*   **Asynchronous Buffering:** Events are stored in a queue, allowing correct ordering and buffering.
*   **One-to-Many:** A single event can be broadcast to multiple listeners.
*   **Optional Data:** An event carries an event code and, optionally, a copy of some data. An event
    without data is valid.

Component's struture
============================
The Event Bus is composed of an event queue (using the *ez_queue* component) and a linked list of
listeners (using the *ez_linked_list* component).

.. mermaid::

    classDiagram
        class ezEventBus_t {
            +struct Node node
            +ezQueue event_queue
        }
        class ezEventListener_t {
            +struct Node node
            +EVENT_CALLBACK callback
        }
        class EVENT_CALLBACK {
            <<typedef>>
            int (*)(uint32_t, const void*, size_t)
        }

        ezEventBus_t o-- ezEventListener_t : manages
        ezEventBus_t *-- ezQueue : contains
        ezEventListener_t ..> EVENT_CALLBACK : uses

Component's behavior
============================
**External Behavior**

1.  **Initialization**: The user creates an ``ezEventBus_t`` instance and initializes it with a memory buffer.
2.  **Registration**: Listeners (``ezEventListener_t``) are initialized with a callback function and registered to the bus using ``ezEventBus_Listen``.
3.  **Publishing**: Any component can publish execution events using ``ezEventBus_SendEvent``. The
    event code and a copy of the data are stored as one element in the bus's queue. If
    ``event_data`` is ``NULL`` or ``event_data_size`` is ``0``, the event has no data. The function
    returns ``ezSUCCESS`` when the event is queued, ``ezSTATUS_ARG_INVALID`` for a ``NULL`` bus or
    data that is too big, and ``ezFAIL`` when the queue has no room.
4.  **Dispatching**: The system must periodically call ``ezEventBus_Run``. This function delivers
    the events that are in the queue when it is called, in the order they were sent, and notifies all
    registered listeners for each event. For an event without data, the listeners receive ``NULL``
    and a size of ``0``.
5.  **Unregistration and reset**: ``ezEventBus_Unlisten`` removes one listener from the bus.
    ``ezEventBus_ResetBus`` unlinks all listeners, so they can listen again, and drops the events
    that are waiting in the queue.

**Rules for the listeners**

*   A listener can listen to a bus only once. ``ezEventBus_Listen`` returns ``ezFAIL`` if the listener
    is already on the bus. A listener must not be on more than one bus at the same time.
*   The data passed to the callback is only valid during the callback. Copy it to keep it.
*   A callback can call ``ezEventBus_SendEvent``. The new event is queued and delivered by the **next**
    call of ``ezEventBus_Run``, not by the current one. This also means a callback that always sends an
    event cannot block ``ezEventBus_Run``.
*   A callback must **not** call ``ezEventBus_Unlisten`` or ``ezEventBus_ResetBus`` on the bus that is
    delivering the event. The delivery walks the listener list and uses the event data of the queue, so
    both would be modified while they are in use.
*   The bus does not protect its queue. ``ezEventBus_SendEvent`` and ``ezEventBus_Run`` must not run
    at the same time, for example from different tasks or from an interrupt. Serialize the calls
    with a lock or a critical section if needed.

**Internal Behavior**

The internal processing loop (``ezEventBus_Run``) follows this sequence:

.. mermaid::

    sequenceDiagram
        participant App
        participant EventBus
        participant Queue
        participant Listener

        loop Every App Cycle
            App->>EventBus: ezEventBus_Run()
            EventBus->>Queue: Get number of events (N)
            loop N times
                EventBus->>Queue: Get front event (header + data)

                loop For Each Listener
                    EventBus->>Listener: callback(code, data, data_size)
                end

                EventBus->>Queue: Pop event
            end
        end

The event stays in the queue until all the listeners have been notified, so they can use its data
during the callback. Only the events that were queued when ``ezEventBus_Run`` started (N) are
delivered. The events that the listeners send during the delivery are delivered by the next call.

Each event is stored as one queue element:

.. code-block:: text

    | event header (aligned)             | event data (event_data_size bytes) |
    | event_code | event_data_size | ... |                                    |

Data Flow
============================
The data flows from the publisher into the queue, and then to the subscribers during the run cycle.

.. mermaid::

    flowchart LR
        Pub[Publisher] -->|ezEventBus_SendEvent| Queue[(Event Queue)]
        subgraph EventBus
            Queue -->|Pop| Dispatcher
            List[Listener List] -.-> Dispatcher
        end
        Dispatcher -->|Callback| Sub1[Subscriber 1]
        Dispatcher -->|Callback| Sub2[Subscriber 2]

Component's data type
============================
The component defines the following key types in ``ez_event_bus.h``:

*   ``ezEventBus_t``: The main structure containing the queue and listener list.
*   ``ezEventListener_t``: The listener structure containing the callback.
*   ``EVENT_CALLBACK``: Function pointer type for event handlers:
    ``typedef int (*EVENT_CALLBACK)(uint32_t event_code, const void *data, size_t data_size);``
    The return value is currently not used by the event bus.
