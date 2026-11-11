/** @file   ez_task.hpp
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Common base of the tasks of the platform layer
 *
 *  @details Only available when EZ_PLATFORM is 1. Every task type exists exactly once: a task
 *  derives from ez::platform::ezTask<Derived> (CRTP) and the base provides the one static
 *  instance, Init(), Entry() and the locking helper. The derived class implements Run() and,
 *  if it has more to set up, OnInit(). There is no virtual function and no object pointer
 *  to carry around. All memory, the signal or delay and the optional mutex are supplied by
 *  the caller.
 */
#pragma once


/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_PLATFORM == 1)

#include <cstdint>
#include "ez_event_bus.h"
#include "ez_sync.hpp"

namespace ez {
namespace platform
{

/** @brief Result codes returned by the platform layer functions */
enum class PlatformResult : uint8_t
{
    OK,                         /**< Operation success */
    Failed,                     /**< Operation failed */
    InvalidParams,              /**< Invalid argument or configuration */
    EventBusNotCreated,         /**< The event bus could not be created or is not available */
    EventListenerNotCreated,    /**< The event listener could not be created */
    DataModelNotCreated,        /**< The data model could not be created or is not available */
    TaskNotConfigured,          /**< Platform::Init() has not succeeded */
    Timeout,                    /**< Could not get the lock in time */
    NoData,                     /**< Nothing to report. No function of the current code returns it */
};

constexpr uint32_t EZ_DEFAULT_LOCK_TIMEOUT_MS = 500;   /**< Used when lock_timeout_ms is 0 */
constexpr uint32_t EZ_DEFAULT_TASK_DELAY_MS   = 1000;  /**< Used when task_delay_ms is 0 */

/** @brief Configuration of a task, see ezTask::Init()
 *  @details Exactly one of wake_signal and delay must be set: it is how the task sleeps
 *           between two passes of Run(). The struct is copied by Init(), the objects it
 *           points to are not copied and must stay valid while the task runs.
 */
typedef struct {
    Mutex *mutex;                   /**< Optional. Protects the data the task shares with other tasks, NULL = nothing to lock. For the platform this is the event bus mutex, which must be recursive (see Mutex) */
    uint32_t lock_timeout_ms;       /**< Max time to wait for the mutex in milliseconds, 0 = EZ_DEFAULT_LOCK_TIMEOUT_MS */
    uint32_t task_delay_ms;         /**< In milliseconds. With delay: sleep between two passes. With wake_signal: longest sleep, Wake() ends it earlier. 0 = EZ_DEFAULT_TASK_DELAY_MS */
    Signal *wake_signal;            /**< Event-driven task: sleeps on it until Wake() notifies it or the timeout expires. Set this or delay, not both */
    Delay *delay;                   /**< Periodic task: sleeps for task_delay_ms, nothing wakes it early. Set this or wake_signal, not both */
}ezTaskConfiguration_t;

/** @brief Lifecycle state of a task */
enum class ezTaskState : uint8_t
{
    Uninitialized,  /**< Init() has not succeeded, Entry() does not run the task */
    Initialized,    /**< Init() succeeded */
};

struct TaskTestAccess;  /* test-only access to the private members */

/** @brief Part of the task that does not depend on the derived class
 *  @details Implemented once in ez_task.cpp, so the code does not grow with the number of tasks.
 *           Use ezTask<Derived>, not this class. The members of this class are called by ezTask<Derived>.
 *  @see ezTask
 */
class ezTaskCore
{
public:
    /** @brief Validate and store the configuration, run on_init, register the listener
     *  @details Exactly one of conf.wake_signal and conf.delay must be set. The defaults of
     *           lock_timeout_ms and task_delay_ms are resolved here. The state is set to
     *           uninitialized first. Results are the same as in ezTask::Init().
     *  @param[in]  conf: Configuration of the task, copied
     *  @param[in]  on_event: Event callback, NULL = the task does not listen
     *  @param[in]  on_init: Hook of the derived class, may be NULL
     *  @return     PlatformResult, see ezTask::Init()
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see ezTask::Init
     */
    PlatformResult Init(const ezTaskConfiguration_t &conf,
                        EVENT_CALLBACK on_event,
                        PlatformResult (*on_init)());

    /** @brief Check whether Init() succeeded
     *  @return true if Init() succeeded
     *
     *  @isr_safe{yes}
     *  @blocking{no}
     */
    bool IsInitialized() const { return state == ezTaskState::Initialized; }

    /** @brief Mark the task as not initialized, for derived classes that validate before calling Init()
     *  @details Only the state is changed. A listener registered by an earlier Init() stays on the event bus.
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    void Invalidate() { state = ezTaskState::Uninitialized; }

    /** @brief Check at the start of Entry(), logs an error if the task is not initialized
     *  @return true if the task can run, false if Init() has not succeeded
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    bool BeginEntry() const;

    /** @brief Sleep between two passes: wait on the signal or sleep with the delay
     *  @details With a wake signal the sleep ends early when Wake() notifies the signal, otherwise
     *           after task_delay_ms. With a delay the task always sleeps task_delay_ms.
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     */
    void Sleep();

    /** @brief Wake the task if it sleeps on a signal
     *  @return true if the signal was set, false if the task uses a delay or the call failed
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    bool Wake();

    /** @brief Take the mutex, waits up to lock_timeout_ms
     *  @details Returns true at once when the configuration has no mutex.
     *  @return true if the mutex is taken or there is none, false on timeout
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Unlock
     */
    bool Lock();

    /** @brief Release the mutex, nothing to do if there is no mutex
     *  @details Logs an error if the mutex cannot be released. Only the task that took the mutex may release it.
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Lock
     */
    void Unlock();

private:
    friend struct TaskTestAccess;

    ezTaskConfiguration_t config{};
    ezEventListener_t listener{};
    volatile ezTaskState state = ezTaskState::Uninitialized;
};

/** @brief Base class of the platform tasks, one static instance per task type
 *  @details The derived class must declare `friend class ez::platform::ezTask<Derived>;`, because the
 *           base calls its Run() and OnInit(). It implements `void Run()`: one pass of the task body,
 *           it must not block and must not loop forever; Entry() sleeps between two passes. It may
 *           implement `PlatformResult OnInit()`: setup that only this task needs. It runs before the
 *           listener is registered and may run again after a failed Init(). It must not declare its
 *           own constructor with arguments, because the instance is created statically.
 *
 *           The task is used through its static members, e.g. SensorTask::Init(config). There is
 *           no virtual function and no object pointer.
 *
 *  \b Example
 *  @code
 *  class SensorTask : public ez::platform::ezTask<SensorTask>
 *  {
 *      friend class ez::platform::ezTask<SensorTask>;
 *
 *      SensorTask() = default;
 *      void Run();
 *  };
 *  @endcode
 *
 *  @tparam Derived: The derived task class
 */
template <class Derived>
class ezTask
{
public:
    /** @brief Get the one and only instance of the task
     *  @details Returns a reference only. The ISR rules of the calls made through it are those of each call.
     *  @return Reference to the instance
     *
     *  @isr_safe{yes}
     *  @blocking{no}
     */
    static Derived& instance() noexcept { return s_instance; }

    /** @brief Configure the task and make it ready to run
     *  @details Validates conf, resolves the default timeouts, calls OnInit(), creates and
     *           registers the event listener on the platform event bus (only if on_event is not NULL),
     *           and marks the task as initialized. All or nothing: on failure the task stays
     *           uninitialized and Entry() does not run it. A new call replaces the previous configuration.
     *
     *           The listener is registered with Platform::RegisterListener(), which waits for the
     *           platform event bus mutex up to lock_timeout_ms. Any result of OnInit() other than OK
     *           is returned unchanged.
     *
     *  @param[in]  conf: Signal or delay, optional mutex and timing of the task
     *  @param[in]  on_event: Callback of the task for the platform events, NULL = no listener
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                      Task initialized
     *  @retval PlatformResult::InvalidParams           Both or none of conf.wake_signal and conf.delay are set
     *  @retval PlatformResult::EventListenerNotCreated ezEventBus_CreateListener() failed
     *  @retval PlatformResult::TaskNotConfigured       on_event is given but Platform::Init() has not succeeded
     *  @retval PlatformResult::Timeout                 The event bus mutex could not be taken in time
     *  @retval PlatformResult::Failed                  The listener could not be registered, e.g. it is already on the bus
     *
     *  @pre Platform::Init() has succeeded if on_event is not NULL. Not called while the task runs,
     *       because Init() writes the configuration without a lock
     *  @post On OK, IsInitialized() is true and Entry() runs the task. On failure, IsInitialized() is false
     *  @warning A listener registered by an earlier Init() stays on the event bus. Init() does not
     *           unregister it, and a new Init() with on_event returns Failed while it is still there.
     *           The new on_event also replaces the callback of that listener before the call fails.
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Entry
     *  @see Wake
     *
     *  \b Example
     *  @code
     *  static MyMutex logging_mutex;     // implements ez::platform::Mutex
     *  static MySignal logging_signal;   // implements ez::platform::Signal
     *  static const ezTaskConfiguration_t logging_config = {
     *      .mutex = &logging_mutex,
     *      .lock_timeout_ms = 100,
     *      .task_delay_ms = 5000,
     *      .wake_signal = &logging_signal,
     *  };
     *  if(LoggingTask::Init(logging_config, LoggingTask::OnEvent) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    static PlatformResult Init(const ezTaskConfiguration_t &conf, EVENT_CALLBACK on_event = nullptr)
    {
        return core.Init(conf, on_event, &OnInitThunk);
    }

    /** @brief Entry point of the task, hand it to the RTOS. The RTOS argument is not used
     *  @details If the task is not initialized it logs an error and returns, so create the
     *           RTOS task only after Init() returned OK. Otherwise it never returns: it
     *           calls Run(), then sleeps on the wake signal or the delay, again and again.
     *
     *  @param[in]  arg: Not used
     *
     *  @pre Init() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Init
     *
     *  \b Example
     *  @code
     *  if(SensorTask::Init(sensor_config) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  if(xTaskCreate(SensorTask::Entry, "sensor", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    static void Entry(void *arg)
    {
        (void)arg;
        if(core.BeginEntry() == false)
        {
            return;
        }

        for(;;)
        {
            s_instance.Run();
            core.Sleep();
        }
    }

    /** @brief Wake the task, so it runs again without waiting for the timeout
     *  @details Notifies the wake signal given in the configuration. It is not for interrupts: from an
     *           interrupt, call NotifyFromISR() on the Signal object that was given as wake_signal.
     *  @return true if the wake signal was set, false if the task sleeps with a delay or the call failed
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Signal::NotifyFromISR
     */
    static bool Wake() { return core.Wake(); }

protected:
    ezTask() = default;
    ezTask(const ezTask&) = delete;
    ezTask& operator=(const ezTask&) = delete;

    /** @brief Check whether Init() succeeded
     *  @return true if Init() succeeded
     *
     *  @isr_safe{yes}
     *  @blocking{no}
     */
    static bool IsInitialized() { return core.IsInitialized(); }

    /** @brief Mark the task as not initialized
     *  @details Call it first when the derived class validates before calling Init(), so a failed
     *           validation does not leave a previous initialization running. Only the state is
     *           changed: a listener registered by an earlier Init() stays on the event bus.
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    static void Invalidate() { core.Invalidate(); }

    /** @brief Setup that only the derived task needs, called by Init(). The default does nothing and returns OK
     *  @details It runs inside Init(), before the listener is registered. It runs again on every
     *           Init() call, also after a failed one. The derived class overrides it by declaring
     *           its own OnInit() with the same signature.
     *  @return PlatformResult: OK if the setup succeeded. Any other value makes Init() fail with it
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     */
    PlatformResult OnInit() { return PlatformResult::OK; }

    /** @brief Takes the task mutex for the lifetime of the object
     *  @details Without a mutex in the configuration, Locked() is true and nothing is locked.
     *           Check Locked() before using the protected data. Never block while the guard
     *           lives: the mutex is held for that time.
     *
     *  \b Example
     *  @code
     *  bool Push(const void *data, size_t size)
     *  {
     *      LockGuard lock;
     *      if(lock.Locked() == false)
     *      {
     *          return false;
     *      }
     *      // use the protected data
     *      return true;
     *  }
     *  @endcode
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     */
    class LockGuard
    {
    public:
        /** @brief Take the task mutex, waits up to lock_timeout_ms */
        LockGuard() : locked(core.Lock()) {}

        /** @brief Release the task mutex, only if the constructor took it */
        ~LockGuard()
        {
            if(locked == true)
            {
                core.Unlock();
            }
        }
        LockGuard(const LockGuard&) = delete;
        LockGuard& operator=(const LockGuard&) = delete;

        /** @brief Check whether the guard holds the mutex
         *  @return true if the mutex is taken or there is none, false on timeout
         */
        bool Locked() const { return locked; }
    private:
        const bool locked;
    };

private:
    friend struct TaskTestAccess;

    static PlatformResult OnInitThunk() { return s_instance.OnInit(); }

    static Derived s_instance;
    static ezTaskCore core;
};

/* Namespace-scope data: no __cxa_guard_* calls on bare-metal toolchains */
template <class Derived>
Derived ezTask<Derived>::s_instance;

template <class Derived>
ezTaskCore ezTask<Derived>::core;

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
