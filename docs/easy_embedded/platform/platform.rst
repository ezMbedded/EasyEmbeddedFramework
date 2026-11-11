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
The application gives the platform a few small objects (mutexes, a signal and, for periodic tasks, a
delay) that wrap the services of its RTOS.

**Key Features:**

*   **One call setup:** ``Platform::Init`` creates the event bus and the data model, and it is all
    or nothing.
*   **Event driven:** The platform task sleeps until an event is sent. It does not poll.
*   **No dynamic allocation:** The application provides every buffer.
*   **RTOS independent:** The platform only uses the ``Mutex``, ``Signal`` and ``Delay`` interfaces.
*   **One instance per task type:** ``Platform`` and the application tasks derive from
    ``ezTask<Derived>``. The type provides the one static instance, so there is no object pointer to
    carry around, and no virtual function.
*   **Application controlled locking for the data model:** The application decides when the data
    model is protected, so it can read several data points under a single lock.

Component's structure
============================
.. mermaid::

    classDiagram
        class ezTask~Derived~ {
            +instance()$ Derived
            +Init(ezTaskConfiguration_t, on_event)$ PlatformResult
            +Entry(arg)$ void
            +Wake()$ bool
            #IsInitialized()$ bool
            #OnInit() PlatformResult
            #LockGuard
        }
        class Platform {
            +Init(ezPlatformConfiguration_t) PlatformResult
            +SendEvent()$ PlatformResult
            +RegisterListener()$ PlatformResult
            +UnregisterListener()$ PlatformResult
            +TakeDataModel()$ PlatformResult
            +GiveDataModel()$ PlatformResult
            +Get()$ PlatformResult
            +Set()$ PlatformResult
            +SetDataPoint()$ PlatformResult
            +GetDataPoint()$ void*
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
        class Delay {
            <<interface>>
            +Sleep(time_ms) void
        }
        ezTask <|-- Platform : Derived = Platform
        ezTask o-- Mutex : optional task mutex
        ezTask o-- Signal : or
        ezTask o-- Delay : one way to sleep
        Platform o-- Mutex : data model mutex
        Platform *-- ezEventBus_t
        Platform *-- ezDataModel_t

``Platform`` is a singleton, the application reaches it with ``Platform::instance()``, which
``ezTask<Platform>`` provides. Everything the application calls while the platform runs is a ``static``
function, so it can be called without the instance.

The synchronization objects
============================
The application gives the platform three objects. The mutexes and the signal have different types,
so they cannot be mixed up. A ``Delay`` is only used by periodic tasks, see `Tasks`_.

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
    * - ``task_config.wake_signal``
      - ``Signal``
      - The platform task waits on it. ``SendEvent`` notifies it. It is a binary signal: it starts
        cleared, and several notifications before the task wakes up count as one.

``Mutex``, ``Signal`` and ``Delay`` are declared in ``ez_sync.hpp``:

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

    class Delay {
    public:
        virtual void Sleep(std::uint32_t time_ms) = 0;
    };

``EZ_SEM_NO_WAIT`` and ``EZ_SEM_WAIT_FOREVER`` are the special values of the timeout.

Configuration
============================
``Platform::Init`` takes an ``ezPlatformConfiguration_t``:

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
    * - ``task_config.wake_signal``
      - The signal that wakes the platform task. Required. ``task_config.delay`` must be ``NULL``,
        the platform task is event driven.
    * - ``datamodel_mutex``
      - The mutex of the data model. Required, different from the task mutex.
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
**Init**

1.  The application fills an ``ezPlatformConfiguration_t`` and calls ``Platform::instance().Init``.
2.  ``Init`` checks the configuration, then creates the event bus and the data model.
3.  If anything fails, nothing stays half created: the platform is left unconfigured, and every
    function, including ``Entry``, refuses to run.
4.  The application registers its listeners with ``Platform::RegisterListener``.
5.  The application creates the RTOS task with ``Platform::Entry`` as the entry function. This
    must be done **after** ``Init`` succeeded.

``Init`` returns:

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Result
      - Meaning
    * - ``OK``
      - The platform is ready.
    * - ``InvalidParams``
      - A mutex or the wake signal is ``NULL``, a delay is set in ``task_config``, both mutexes are
        the same object, the event bus memory is missing, or the data model memory is only partly
        provided.
    * - ``EventBusNotCreated``
      - The event bus rejected its memory, for example because the buffer is too small.
    * - ``DataModelNotCreated``
      - The data model rejected its table or buffer.

``Init`` is meant to be called once, before the platform task runs. Calling it again creates a new
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
*   ``TaskNotConfigured``: ``Init`` did not succeed.
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

Tasks
============================
The application tasks use the same base class as the platform: ``ezTask<Derived>`` in ``ez_task.hpp``.
Every task type exists exactly once, so the base provides the static instance and the functions that
hand the task to the RTOS.

A task derives from ``ezTask<Derived>`` and implements ``Run()``, one pass of the task body. If it
has more to set up it also implements ``OnInit()``. This is the whole sensor task of the Raspberry Pi
Pico sample, a periodic task:

.. code-block:: cpp

    class SensorTask : public platform::ezTask<SensorTask>
    {
        friend class platform::ezTask<SensorTask>;
        SensorTask() = default;
        void Run();                 /* read the sensor, store the value, send the event */
    };

The application configures every task with one call and hands ``Entry`` to the RTOS:

.. code-block:: cpp

    static AppDelay sensor_delay;
    static const ezTaskConfiguration_t sensor_config = {
        .task_delay_ms = 3000,
        .delay = &sensor_delay,
    };

    if(SensorTask::Init(sensor_config) != PlatformResult::OK) { /* handle the error */ }
    xTaskCreate(SensorTask::Entry, "sensor", 1024, NULL, priority, NULL);

``Init`` validates the configuration, calls ``OnInit()``, registers the event listener (only if an
event callback is given) and marks the task as initialized. It is all or nothing. ``Entry`` runs
``Run()``, then sleeps, again and again. If the task is not initialized it logs an error and returns,
so create the RTOS task only after ``Init`` returned ``OK``.

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - ``ezTaskConfiguration_t``
      - Description
    * - ``wake_signal`` or ``delay``
      - **Exactly one** of them. An event driven task sleeps on a ``Signal``: ``Wake()`` or a
        ``Notify()`` on it ends the sleep at once, ``task_delay_ms`` is only the longest sleep. A
        periodic task sleeps with a ``Delay`` for ``task_delay_ms``.
    * - ``mutex``
      - Optional. Only for a task that shares data with other tasks, ``NULL`` means nothing to
        protect. Inside the task use the nested ``LockGuard``, which takes the mutex for the life
        of the object. Check ``Locked()`` before using the protected data.
    * - ``lock_timeout_ms``
      - Max time to wait for the mutex. ``0`` selects ``EZ_DEFAULT_LOCK_TIMEOUT_MS``.
    * - ``task_delay_ms``
      - ``0`` selects ``EZ_DEFAULT_TASK_DELAY_MS``.

``Run()`` must return: it is one pass, and ``Entry`` does the sleeping. A task that listens to the
platform events passes a callback to ``Init``, for example ``LoggingTask::Init(config,
LoggingTask::OnEvent)``. The callback runs in the platform task and typically stores the event and
calls ``Wake()``, because it is a ``static`` function it reaches the task with ``instance()``. See
``samples/pi-pico`` for the three task types.

Rules
============================
*   Call ``Init`` before anything else, and create the platform task only after it succeeded. If
    ``Entry`` runs on an unconfigured platform, it returns at once. On an RTOS where a task is not
    allowed to return, create the task only when ``Init`` returned ``OK``. The same holds for every
    task and its ``Init``.
*   The task mutex must be recursive. The data model mutex and the task mutex must be different
    objects.
*   The listeners run in the platform task with the task mutex held. Keep them short. The event data
    is only valid during the callback. The rules of the :doc:`event bus
    <../service/event_bus/event_bus>` apply, for example a listener must not unregister itself.
*   The memory given to ``Init`` and the synchronization objects must live as long as the platform
    task runs.
*   Do not call the platform functions that wait for a mutex from an interrupt.

Porting to an RTOS
============================
Wrap the services of the RTOS in a ``Mutex`` and a ``Signal``, and a ``Delay`` if you have periodic
tasks. This is an example for FreeRTOS:

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

    class FreeRtosDelay : public Delay {
    public:
        void Sleep(std::uint32_t time_ms) override { vTaskDelay(pdMS_TO_TICKS(time_ms)); }
    };

Then create the objects with static storage, call ``Init``, and give ``Platform::Entry`` to
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
      - ``Init`` did not succeed.
    * - ``EventListenerNotCreated``
      - ``ezTask::Init`` could not create the event listener of the task.
    * - ``EventBusNotCreated``
      - ``Init`` could not create the event bus.
    * - ``DataModelNotCreated``
      - There is no data model, or ``Init`` could not create it.
    * - ``Timeout``
      - A mutex was not available in time. The call can be retried.
    * - ``NoData``
      - There is nothing to report.
