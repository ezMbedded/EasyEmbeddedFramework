============================================================
Platform
============================================================

Introduction
============================
The **Platform** component is the part of the framework that the firmware application runs on top
of. It owns two things that every application needs and that several tasks share:

*   an **event bus**, so the tasks of the application can send events to each other, and
*   a **data model**, so the tasks can share data points.

The platform runs the event bus in its own RTOS task. It does not depend on a particular RTOS.
The application gives the platform a few small objects (a mutex and a signal) that wrap the
services of its RTOS.

**Key Features:**

*   **One call setup:** ``Platform::Setup`` creates the event bus and the data model, and it is all
    or nothing.
*   **Event driven:** The platform task sleeps until an event is sent. It does not poll.
*   **No dynamic allocation:** The application provides every buffer.
*   **RTOS independent:** The platform only uses the ``Mutex`` and ``Signal`` interfaces.
*   **Application controlled locking for the data model:** The application decides when the data
    model is protected, so it can read several data points under a single lock.

Component's structure
============================
.. mermaid::

    classDiagram
        class Task {
            +Setup(ezTaskConfiguration_t) PlatformResult
            +Stop()*
            +Start()*
            #Run(context)*
        }
        class Platform {
            +instance()$ Platform
            +Setup(ezPlatformConfiguration_t) PlatformResult
            +SendEvent()$ PlatformResult
            +RegisterListener()$ PlatformResult
            +UnregisterListener()$ PlatformResult
            +TakeDataModel()$ PlatformResult
            +GiveDataModel()$ PlatformResult
            +Get()$ PlatformResult
            +Set()$ PlatformResult
            +SetDataPoint()$ PlatformResult
            +GetDataPoint()$ void*
            +TaskEntry()$ void
        }
        class Mutex {
            <<interface>>
            +Lock(timeout_ms) bool
            +Unlock() bool
        }
        class Signal {
            <<interface>>
            +Wait(timeout_ms) bool
            +Notify() bool
            +NotifyFromISR() bool
        }
        Task <|-- Platform
        Platform o-- Mutex : task mutex
        Platform o-- Mutex : data model mutex
        Platform o-- Signal : wake signal
        Platform *-- ezEventBus_t
        Platform *-- ezDataModel_t

``Platform`` is a singleton, the application reaches it with ``Platform::instance()``. Everything
the application calls while the platform runs is a ``static`` function, so it can be called without
the instance.

The synchronization objects
============================
The application gives the platform three objects. They have two different types, so they cannot be
mixed up.

.. list-table::
    :header-rows: 1
    :widths: 20 12 68

    * - Object
      - Type
      - Purpose
    * - ``task_config.mutex``
      - ``Mutex``
      - Protects the event bus. It **must be recursive**: the event listeners run with this mutex
        held and may call ``SendEvent`` or ``RegisterListener``.
    * - ``datamodel_mutex``
      - ``Mutex``
      - Protects the data model. It must be a different object from the task mutex, so a long read
        of the data model never blocks the event bus.
    * - ``wake_signal``
      - ``Signal``
      - The platform task waits on it. ``SendEvent`` notifies it. It is a binary signal: it starts
        cleared, and several notifications before the task wakes up count as one.

``Mutex`` and ``Signal`` are declared in ``ez_sync.hpp``:

.. code-block:: cpp

    class Mutex {
    public:
        virtual bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;
        virtual bool Unlock() = 0;
    };

    class Signal {
    public:
        virtual bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;
        virtual bool Notify() = 0;
        virtual bool NotifyFromISR() = 0;
    };

``EZ_SEM_NO_WAIT`` and ``EZ_SEM_WAIT_FOREVER`` are the special values of the timeout.

Configuration
============================
``Platform::Setup`` takes an ``ezPlatformConfiguration_t``:

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Field
      - Description
    * - ``task_config.mutex``
      - The recursive mutex of the event bus. Required.
    * - ``task_config.lock_timeout_ms``
      - Max time the platform waits for the task mutex. ``0`` selects
        ``EZ_DEFAULT_LOCK_TIMEOUT_MS`` (500 ms).
    * - ``task_config.task_delay_ms``
      - Max time the platform task sleeps while it waits for an event. ``0`` selects
        ``EZ_DEFAULT_TASK_DELAY_MS`` (1000 ms). Events wake the task at once, this is only the
        longest it sleeps when nothing happens.
    * - ``datamodel_mutex``
      - The mutex of the data model. Required, different from the task mutex.
    * - ``wake_signal``
      - The signal that wakes the platform task. Required.
    * - ``event_bus.buffer``, ``event_bus.size``
      - Memory of the event queue. Required.
    * - ``data_model.data_points``, ``data_model.num_of_data_points``
      - Table of the data points.
    * - ``data_model.buffer``, ``data_model.buffer_size``
      - Memory that stores the values of the data points.

The data model is optional. If all four ``data_model`` fields are ``0`` or ``NULL``, the platform
runs without a data model, and the data model functions return ``DataModelNotCreated``. Providing
only some of the fields is an error.

Component's behavior
============================
**Setup**

1.  The application fills an ``ezPlatformConfiguration_t`` and calls ``Platform::instance().Setup``.
2.  ``Setup`` checks the configuration, then creates the event bus and the data model.
3.  If anything fails, nothing stays half created: the platform is left unconfigured, and every
    function, including ``TaskEntry``, refuses to run.
4.  The application registers its listeners with ``Platform::RegisterListener``.
5.  The application creates the RTOS task with ``Platform::TaskEntry`` as the entry function. This
    must be done **after** ``Setup`` succeeded.

``Setup`` returns:

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Result
      - Meaning
    * - ``OK``
      - The platform is ready.
    * - ``InvalidParams``
      - A mutex or the signal is ``NULL``, both mutexes are the same object, the event bus memory
        is missing, or the data model memory is only partly provided.
    * - ``EventBusNotCreated``
      - The event bus rejected its memory, for example because the buffer is too small.
    * - ``DataModelNotCreated``
      - The data model rejected its table or buffer.

``Setup`` is meant to be called once, before the platform task runs. Calling it again creates a new
event bus and a new data model, and the registered listeners are lost.

**Running**

.. mermaid::

    sequenceDiagram
        participant App as Application task
        participant P as Platform
        participant PT as Platform task
        participant L as Listener

        App->>P: SendEvent(code, data)
        P->>P: lock task mutex, queue the event, unlock
        P->>PT: Notify wake signal
        PT->>PT: Wait returns
        PT->>P: lock task mutex
        PT->>L: callback(code, data, size)
        PT->>P: unlock task mutex
        PT->>PT: Wait(task_delay_ms)

The platform task handles all the queued events, then sleeps in ``Wait`` until the next
``SendEvent`` or until ``task_delay_ms`` has passed. It does not use CPU time while it has nothing
to do.

``Stop`` makes the platform task skip the event handling, ``Start`` makes it handle the events
again. The events sent while it is stopped stay in the queue.

**Events**

``SendEvent(code, data, size)`` copies the data into the event queue. The data pointer may be
``NULL`` if the size is ``0``. The function returns:

*   ``OK``: the event is queued and the platform task is woken.
*   ``TaskNotConfigured``: ``Setup`` did not succeed.
*   ``InvalidParams``: the data is ``NULL`` but the size is not ``0``.
*   ``Timeout``: the task mutex is not available within ``lock_timeout_ms``. It can be retried.
*   ``Failed``: the event does not fit into the queue.

``SendEvent`` waits for a mutex, so it must **not** be called from an interrupt.

**The data model**

The data model functions do not lock by themselves. The application decides when to protect them,
because only the application knows if it reads one value or several.

*   For a single value, use the typed helpers. They lock, check the size, copy, and unlock:

    .. code-block:: cpp

        float temperature = 21.5f;
        Platform::Set(DATA_POINT_TEMPERATURE, temperature);

        float read_value = 0.0f;
        if(Platform::Get(DATA_POINT_TEMPERATURE, read_value) == PlatformResult::OK)
        {
            /* use read_value */
        }

    ``Get`` returns ``InvalidParams`` if ``sizeof(T)`` is not the size of the data point. ``T`` must
    be trivially copyable, arrays are allowed. Do not call them while holding the data model lock.

*   For several data points, or to keep a pointer, lock the data model yourself:

    .. code-block:: cpp

        if(Platform::TakeDataModel(50) == PlatformResult::OK)
        {
            size_t size = 0;
            const void *data = Platform::GetDataPoint(DATA_POINT_TEMPERATURE, &size);
            /* data is stable until GiveDataModel(), nobody can set it */
            Platform::GiveDataModel();
        }

    Keep the lock short, and do not call ``SendEvent`` or any function that blocks while holding it.

*   To save the data points that changed, do the whole sequence under **one** lock. If the lock is
    released between reading and clearing a dirty flag, a data point that was set in between loses
    its flag without being saved:

    .. code-block:: cpp

        if(Platform::TakeDataModel(50) == PlatformResult::OK)
        {
            uint32_t index = Platform::GetFirstDirtyDataIndex();
            while(index != DATA_POINT_INVALID)
            {
                size_t size = 0;
                const void *data = Platform::GetDataPoint(index, &size);
                save(index, data, size);
                Platform::ClearDirtyData(index);
                index = Platform::GetFirstDirtyDataIndex();
            }
            Platform::GiveDataModel();
        }

Rules
============================
*   Call ``Setup`` before anything else, and create the platform task only after it succeeded. If
    ``TaskEntry`` runs on an unconfigured platform, it returns at once. On an RTOS where a task is not
    allowed to return, create the task only when ``Setup`` returned ``OK``.
*   The task mutex must be recursive. The data model mutex and the task mutex must be different
    objects.
*   The listeners run in the platform task with the task mutex held. Keep them short. The event data
    is only valid during the callback. The rules of the :doc:`event bus
    <../service/event_bus/event_bus>` apply, for example a listener must not unregister itself.
*   The memory given to ``Setup`` and the synchronization objects must live as long as the platform
    task runs.
*   Do not call the platform functions that wait for a mutex from an interrupt.

Porting to an RTOS
============================
Wrap the services of the RTOS in a ``Mutex`` and a ``Signal``. This is an example for FreeRTOS:

.. code-block:: cpp

    class FreeRtosMutex : public Mutex {
    public:
        FreeRtosMutex() : handle(xSemaphoreCreateRecursiveMutex()) {}

        bool Lock(std::uint32_t timeout_ms) override
        {
            TickType_t ticks = (timeout_ms == EZ_SEM_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
            return xSemaphoreTakeRecursive(handle, ticks) == pdTRUE;
        }

        bool Unlock() override { return xSemaphoreGiveRecursive(handle) == pdTRUE; }
    private:
        SemaphoreHandle_t handle;
    };

    class FreeRtosSignal : public Signal {
    public:
        FreeRtosSignal() : handle(xSemaphoreCreateBinary()) {}

        bool Wait(std::uint32_t timeout_ms) override
        {
            TickType_t ticks = (timeout_ms == EZ_SEM_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
            return xSemaphoreTake(handle, ticks) == pdTRUE;
        }

        bool Notify() override { return xSemaphoreGive(handle) == pdTRUE; }

        bool NotifyFromISR() override
        {
            BaseType_t woken = pdFALSE;
            bool result = (xSemaphoreGiveFromISR(handle, &woken) == pdTRUE);
            portYIELD_FROM_ISR(woken);
            return result;
        }
    private:
        SemaphoreHandle_t handle;
    };

Then create the objects with static storage, call ``Setup``, and give ``Platform::TaskEntry`` to
``xTaskCreate``. A complete example that runs on Linux, with ``std::thread`` instead of an RTOS,
is in ``samples/linux_platform/main.cpp``.

Result codes
============================
.. list-table::
    :header-rows: 1
    :widths: 28 72

    * - ``PlatformResult``
      - Meaning
    * - ``OK``
      - The call succeeded.
    * - ``Failed``
      - The call failed, for example an unknown data point index, or an event that is too big.
    * - ``InvalidParams``
      - An argument is invalid.
    * - ``TaskNotConfigured``
      - ``Setup`` did not succeed.
    * - ``EventBusNotCreated``
      - ``Setup`` could not create the event bus.
    * - ``DataModelNotCreated``
      - There is no data model, or ``Setup`` could not create it.
    * - ``Timeout``
      - A mutex was not available in time. The call can be retried.
    * - ``NoData``
      - There is nothing to report.
