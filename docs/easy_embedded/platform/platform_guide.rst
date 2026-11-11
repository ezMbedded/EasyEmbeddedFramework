============================================================
Platform and Task: User Guide
============================================================

This guide explains how to build a firmware application on the **Platform** and the **ezTask**
base class. It starts with the ideas behind the design, then shows how to use it, step by step,
with the code of ``samples/pi-pico``. The reference of every function and field is in
:doc:`platform`, and the API cheat sheets are in ``docs_llm/api/``.

Philosophy of the design
============================

The framework is made for small MCUs, so the design follows a few rules. Knowing them makes the API
easy to predict.

**The application owns the memory, the framework owns the rules.**
    There is no dynamic allocation. Every buffer, mutex and signal is a ``static`` object that the
    application creates and gives to the framework. The framework only keeps a pointer.

**The framework does not know the RTOS.**
    The platform needs three tiny interfaces (``Mutex``, ``Signal`` and ``Delay``, in
    ``ez_sync.hpp``). The application implements them once with the calls of its RTOS. They are
    three different types, so a mutex cannot be passed where a signal is expected.

**One task type, one instance.**
    In an embedded system each task body exists once. ``ezTask<Derived>`` uses the *curiously
    recurring template pattern* (CRTP): the type provides the one static instance, the base calls
    ``Derived::Run()`` directly. The result is no object pointer to carry around, no ``this`` to
    pass to the RTOS, no vtable in the flash and no heap. A callback, being a ``static`` function,
    reaches its task with ``instance()``.

**Tasks sleep, they do not poll.**
    An event driven task waits on a ``Signal`` and is woken at once by an event. A periodic task
    sleeps with a ``Delay``. The timeout is only a fallback, so a quiet system uses no CPU.

**The base class does the boring part.**
    The part that every task needs and that is easy to get wrong lives in the base class:
    configuration with defaults, the order of the initialization, the loop and the sleep of
    ``Entry``, a check that the task was initialized, the registration of the event listener and
    the locking. The task only writes ``Run()`` and, when needed, ``OnInit()``.

**Failing is loud and early.**
    ``Init`` reports a wrong configuration, and a task that was not initialized refuses to run
    instead of dereferencing a null pointer. Every function returns a result and none of them may
    be ignored.

Overview
============================

.. mermaid::

    flowchart LR
        S[Sensor task<br/>periodic, Delay] -->|SendEvent| B((Platform<br/>event bus))
        B -->|OnEvent in platform task| C[Climate control task<br/>event driven, Signal]
        B -->|OnEvent in platform task| L[Logging task<br/>event driven, Signal + Mutex]
        C -->|SendEvent heater/cooler| B

The platform runs the event bus in its own RTOS task. The sample has three application tasks:

.. list-table::
    :header-rows: 1
    :widths: 22 18 20 40

    * - Task
      - Kind
      - Sleeps with
      - What it does
    * - ``SensorTask``
      - periodic
      - ``Delay``
      - Every 3 s stores a temperature in the data model and sends ``EVENT_DATA_POINT_CHANGED``.
    * - ``ClimateControlTask``
      - event driven
      - ``Signal``
      - Reads the temperature when the event arrives and runs a state machine that requests a
        heater or a cooler.
    * - ``LoggingTask``
      - event driven
      - ``Signal`` and ``Mutex``
      - Stores the events in a ring buffer and logs them from its own task, so the platform task is
        never slowed down by the logging.

Using the platform
============================

Step 1: implement the synchronization interfaces
-------------------------------------------------
Wrap your RTOS once. The sample does it in ``samples/pi-pico/porting/``. A ``Signal`` for FreeRTOS:

.. code-block:: cpp

    class AppSignal : public Signal
    {
    public:
        AppSignal() { sem = xSemaphoreCreateBinary(); }

        bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
        {
            TickType_t tick = (timeout_ms == EZ_SEM_WAIT_FOREVER) ? portMAX_DELAY
                                                                 : pdMS_TO_TICKS(timeout_ms);
            return (xSemaphoreTake(sem, tick) == pdTRUE);   /* taking clears it */
        }

        bool Notify() override { /* give the semaphore, "already set" is fine */ }
        bool NotifyFromISR() override { /* xSemaphoreGiveFromISR + portYIELD_FROM_ISR */ }
    private:
        SemaphoreHandle_t sem = nullptr;
    };

and a ``Delay``, which has no state, so a single object can serve all periodic tasks:

.. code-block:: cpp

    class AppDelay : public Delay
    {
    public:
        void Sleep(std::uint32_t time_ms) override { vTaskDelay(pdMS_TO_TICKS(time_ms)); }
    };

``AppMutex`` is a **recursive** mutex, see :doc:`platform` for why.

Step 2: describe the platform and the tasks as static data
-----------------------------------------------------------
``samples/pi-pico/tasks/platform/platform_config.cpp`` keeps all the configuration in one place.
The objects are ``static``, so they live as long as the program:

.. code-block:: cpp

    static AppMutex  task_mutex;        /* protects the event bus */
    static AppMutex  datamodel_mutex;   /* protects the data model, a different object */
    static AppSignal wake_signal;       /* wakes the platform task */

    static uint8_t event_bus_buffer[512];
    static uint8_t data_model_buffer[256];

    static const ezPlatformConfiguration_t config = {
        .task_config = {
            .mutex = &task_mutex,
            .wake_signal = &wake_signal,
        },
        .datamodel_mutex = &datamodel_mutex,
        .event_bus = { .buffer = event_bus_buffer, .size = sizeof(event_bus_buffer) },
        .data_model = {
            .data_points = data_points,
            .num_of_data_points = sizeof(data_points) / sizeof(data_points[0]),
            .buffer = data_model_buffer,
            .buffer_size = sizeof(data_model_buffer),
        },
    };

Each application task gets an ``ezTaskConfiguration_t``. Set **exactly one** of ``wake_signal`` and
``delay``; a field left at ``0`` takes its default (500 ms lock timeout, 1000 ms task delay):

.. code-block:: cpp

    /* periodic: sleeps 3 s with a Delay, needs no mutex and no listener */
    static AppDelay sensor_delay;
    static const ezTaskConfiguration_t sensor_config = {
        .task_delay_ms = 3000,
        .delay = &sensor_delay,
    };

    /* event driven: woken by its listener */
    static AppSignal climate_control_signal;
    static const ezTaskConfiguration_t climate_control_config = {
        .wake_signal = &climate_control_signal,
    };

    /* event driven, and shares data with the platform task: it also needs a mutex */
    static AppMutex  logging_mutex;
    static AppSignal logging_signal;
    static const ezTaskConfiguration_t logging_config = {
        .mutex = &logging_mutex,
        .lock_timeout_ms = 100,
        .task_delay_ms = 5000,          /* only a safety timeout */
        .wake_signal = &logging_signal,
    };

Step 3: initialize, then create the RTOS tasks
-----------------------------------------------
The order is: ``Platform::Init`` first, then ``Init`` of every task, then create the RTOS tasks
with ``Entry`` as the entry function. Create an RTOS task only after its ``Init`` returned ``OK``.

.. code-block:: cpp

    bool appPlatformConfig_Init(void)
    {
        if(Platform::instance().Init(config) != PlatformResult::OK)
        {
            return false;
        }

        if(SensorTask::Init(sensor_config) != PlatformResult::OK
            || ClimateControlTask::Init(climate_control_config, ClimateControlTask::OnEvent) != PlatformResult::OK
            || LoggingTask::Init(logging_config, LoggingTask::OnEvent) != PlatformResult::OK)
        {
            return false;
        }

        if(xTaskCreate(Platform::Entry, "platform", 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
            || xTaskCreate(SensorTask::Entry, "sensor", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
            || xTaskCreate(ClimateControlTask::Entry, "climate", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
            || xTaskCreate(LoggingTask::Entry, "logging", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
        {
            return false;
        }
        return true;
    }

``main`` calls ``appPlatformConfig_Init()`` and then ``vTaskStartScheduler()``.

``Init`` does, in this order: check the configuration, store it, call the task's ``OnInit()``,
create and register the event listener (only if you passed a callback), and mark the task
initialized. The listener comes last, so no event can reach a half initialized task.

Writing a task
============================

A task is a class that derives from ``ezTask`` with **itself** as the template argument. The rules
are:

1.  Derive from ``ezTask<YourTask>`` and declare ``friend class ez::platform::ezTask<YourTask>;``
    so the base can call your private functions.
2.  Make the constructor and destructor private. The base provides the single instance.
3.  Implement ``void Run()``. It is **one pass** of the body and it must return. ``Entry`` calls it,
    sleeps, and calls it again.
4.  Optionally implement ``PlatformResult OnInit()`` to prepare the task's own state. Return
    ``OK`` or an error, and the task does not start on an error.

Periodic task: ``SensorTask``
------------------------------

.. code-block:: cpp

    class SensorTask : public platform::ezTask<SensorTask>
    {
        friend class platform::ezTask<SensorTask>;
        SensorTask() = default;
        ~SensorTask() = default;

        void Run();     /* one pass: read the sensor, store it, tell the others */
    };

    void SensorTask::Run()
    {
        float temperature = randomFloat(-20.0f, 40.0f);
        const uint32_t index = DATA_POINT_TEMPERATURE;

        if(Platform::Set(index, temperature, 100) != PlatformResult::OK)
        {
            EZERROR("cannot store the temperature");
            return;
        }
        if(Platform::SendEvent(EVENT_DATA_POINT_CHANGED, &index, sizeof(index)) != PlatformResult::OK)
        {
            EZERROR("cannot send the temperature event");
        }
    }

There is no loop and no delay in ``Run()``: ``Entry`` sleeps for ``task_delay_ms`` using the
``Delay`` of the configuration.

Event driven task: ``ClimateControlTask``
------------------------------------------
The task reacts to events. The callback ``OnEvent`` is registered by ``Init`` on the platform event
bus. It runs in the **platform task**, so it only does the quick part and hands the rest over with
``Wake()``. ``Run()`` runs in the task's own RTOS task.

.. code-block:: cpp

    class ClimateControlTask : public platform::ezTask<ClimateControlTask>
    {
        friend class platform::ezTask<ClimateControlTask>;
    public:
        static int OnEvent(uint32_t event_code, const void *data, size_t data_size);
    private:
        ClimateControlTask() = default;
        ~ClimateControlTask() = default;
        PlatformResult OnInit();   /* start the state machine */
        void Run();                /* let the state machine handle the posted events */
    };

    int ClimateControlTask::OnEvent(uint32_t event_code, const void *data, size_t data_size)
    {
        /* ... read the temperature with Platform::Get(), decide heating or cooling ... */
        if(!ezSm_PostEvent(&sm, &event))
        {
            EZERROR("cannot post event to state machine");
        }
        else if(Wake() == false)            /* ends the sleep of Entry at once */
        {
            EZERROR("cannot wake the climate control task");
        }
        return 0;
    }

    void ClimateControlTask::Run()
    {
        if(ezSm_Run(&sm) == 0)
        {
            EZTRACE("no event handled");
        }
    }

Because ``OnEvent`` is ``static``, it reaches the task with ``instance()`` when it needs a member.

Event driven task with shared data: ``LoggingTask``
----------------------------------------------------
``OnEvent`` (platform task) pushes into a ring buffer and ``Run()`` (logging task) pops from it. Two
tasks touch the same data, so the configuration gives the task a ``mutex`` and the code uses the
nested ``LockGuard``:

.. code-block:: cpp

    bool LoggingTask::Push(const void *data, size_t size)
    {
        LockGuard lock;                      /* takes the task mutex */
        if(lock.Locked() == false)
        {
            return false;                    /* timeout, do not touch the data */
        }
        return (ezRingBuffer_Push(&this->ring_buff, (uint8_t*)data, (uint16_t)size) == size);
    }                                        /* the guard releases the mutex here */

    void LoggingTask::Run()
    {
        LoggingEvent_t event{};
        /* the lock is held only inside Pop(), never while logging */
        while(Pop(&event, sizeof(LoggingEvent_t)) == true)
        {
            /* log the event */
        }
    }

    PlatformResult LoggingTask::OnInit()     /* called by Init() */
    {
        if(ezRingBuffer_Init(&this->ring_buff, this->buff, sizeof(this->buff)) == false)
        {
            return PlatformResult::Failed;
        }
        return PlatformResult::OK;
    }

Without a mutex in the configuration, ``Locked()`` is always ``true`` and nothing is locked, so the
same code works for a task that has nothing to protect.

Sharing data
============================

The platform has a data model for values that several tasks use. ``Platform::Set`` and
``Platform::Get`` take the data model mutex for the duration of one call:

.. code-block:: cpp

    float temperature = 0.0f;
    if(Platform::Get(DATA_POINT_TEMPERATURE, temperature, 100) != PlatformResult::OK)
    {
        EZERROR("cannot read temperature");
    }

The usual pattern is *store, then announce*: the producer writes the data point, then sends
``EVENT_DATA_POINT_CHANGED`` with the index. The listeners read the value only when they need it.
To read several data points under one lock, use ``Platform::TakeDataModel`` and
``Platform::GiveDataModel``, see :doc:`platform`.

Rules to remember
============================

*   Call ``Platform::Init`` first, ``Init`` of each task second, and create the RTOS task last,
    only after the ``Init`` returned ``OK``. A task that was not initialized returns from ``Entry``
    at once.
*   Set exactly one of ``wake_signal`` and ``delay``. A periodic task uses a ``Delay``, an event
    driven task uses a ``Signal``.
*   ``Run()`` returns. Never put a loop or a delay inside it.
*   ``OnEvent`` runs in the platform task with the event bus mutex held. Keep it short, never block,
    and copy the event data because it is only valid during the callback.
*   Use ``LockGuard`` for the data that two tasks share, and never block while holding it.
*   The task mutex of the platform must be recursive, and the data model mutex must be a different
    object.
*   The configuration objects (mutexes, signals, delay, buffers) must live as long as the task runs.
    Make them ``static``.
*   Check every return value.

Adding a new task: checklist
============================

1.  Create the class, derive from ``ezTask<NewTask>``, add the ``friend`` line and the private
    constructor and destructor.
2.  Write ``Run()``. Add ``OnInit()`` if the task has state to prepare, and a static ``OnEvent`` if it
    listens to events.
3.  In the configuration file, add the ``static`` signal or delay (and a mutex if it shares data) and
    an ``ezTaskConfiguration_t``.
4.  Call ``NewTask::Init(config)`` or ``NewTask::Init(config, NewTask::OnEvent)``, check the result.
5.  Create the RTOS task with ``NewTask::Entry``, check the result.
