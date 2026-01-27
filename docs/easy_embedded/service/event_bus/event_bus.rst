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
            int (*)(uint32_t, void*, size_t)
        }

        ezEventBus_t o-- ezEventListener_t : manages
        ezEventBus_t *-- ezQueue : contains
        ezEventListener_t ..> EVENT_CALLBACK : uses

Component's behavior
============================
**External Behavior**

1.  **Initialization**: The user creates an ``ezEventBus_t`` instance and initializes it with a memory buffer.
2.  **Registration**: Listeners (``ezEventListener_t``) are initialized with a callback function and registered to the bus using ``ezEventBus_Listen``.
3.  **Publishing**: Any component can publish execution events using ``ezEventBus_SendEvent``. This pushes the event code and data into the bus's queue.
4.  **Dispatching**: The system must periodically call ``ezEventBus_Run``. This function processes the queue and notifies all registered listeners for each event.

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
            alt Queue has >= 2 elements (Code + Data)
                EventBus->>Queue: Pop Event Code
                EventBus->>Queue: Pop Event Data
                
                loop For Each Listener
                    EventBus->>Listener: callback(code, data)
                end
            end
        end

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

*   ``ezEventBus_t``: The main subject structure containing the queue and listener list.
*   ``ezEventListener_t``: The observer structure containing the callback.
*   ``EVENT_CALLBACK``: Function pointer type for event handlers:
    ``typedef int (*EVENT_CALLBACK)(uint32_t event_code, void *data, size_t data_size);``
