/** @file   ez_task.hpp
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Common interface for the tasks of the platform layer
 *
 *  @details Only available when EZ_PLATFORM is 1. A task derives from ez::platform::Task,
 *  is configured with Task::Setup() and ezTaskConfiguration_t, and implements Start(),
 *  Stop() and Run(). All memory and the mutex are supplied by the caller.
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
    TaskNotConfigured,          /**< Setup() has not succeeded */
    Timeout,    /**< Could not get the lock in time, can be retried */
    NoData,     /**< Nothing to report, e.g. no dirty data point pending */
};

constexpr uint32_t EZ_DEFAULT_LOCK_TIMEOUT_MS = 500;   /**< Used when lock_timeout_ms is 0 */
constexpr uint32_t EZ_DEFAULT_TASK_DELAY_MS   = 1000;  /**< Used when task_delay_ms is 0 */

/** @brief Configuration of a task, see Task::Setup() */
typedef struct {
    Mutex *mutex;                   /**< Must be recursive: event listeners may call back into the platform */
    uint32_t lock_timeout_ms;       /**< Max time to wait for a mutex, 0 = EZ_DEFAULT_LOCK_TIMEOUT_MS */
    uint32_t task_delay_ms;         /**< Max time the task sleeps while waiting for an event, 0 = EZ_DEFAULT_TASK_DELAY_MS */
}ezTaskConfiguration_t;

/** @brief Base class of the platform tasks
 *  @details Holds the validated configuration and the configured/initialized flags.
 *           The derived class implements Start(), Stop() and Run().
 */
class Task
{
public:
    virtual ~Task() = default;

    /** @brief Stop the task from handling events. Implemented by the derived class */
    virtual void Stop() = 0;

    /** @brief Start or resume the task. Implemented by the derived class */
    virtual void Start() = 0;

    /** @brief Validate and store the task configuration, and mark the task as configured
     *  @details Calls ApplyConfig(). A lock_timeout_ms or task_delay_ms equal to 0 is replaced
     *           by EZ_DEFAULT_LOCK_TIMEOUT_MS or EZ_DEFAULT_TASK_DELAY_MS. A failed call
     *           leaves the task not configured and not initialized, even if it was
     *           configured before.
     *
     *  @param[in]  conf: Mutex and timing of the task. The mutex is owned by the caller
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK              Task configured
     *  @retval PlatformResult::InvalidParams   conf.mutex is NULL
     *
     *  @pre None
     *  @post On success the task is configured; on failure it is not configured
     *  @isr_safe{no}
     *  @blocking{no}
     *
     *  \b Example
     *  @code
     *  ezTaskConfiguration_t task_config = {};
     *  task_config.mutex = &task_mutex;
     *  if(my_task.Setup(task_config) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    PlatformResult Setup(const ezTaskConfiguration_t &conf)
    {
        if(ApplyConfig(conf) != PlatformResult::OK)
        {
            return PlatformResult::InvalidParams;
        }
        is_configured = true;
        return PlatformResult::OK;
    }

protected:
    /** @brief Body of the task, implemented by the derived class
     *  @param[in]  context: User context passed by the code that starts the task
     */
    virtual void Run(void *context) = 0;

    /** @brief Validate and store the configuration without marking the task as configured
     *  @details For the tasks that have more to set up. A failed call also marks the task as
     *           not configured, so an invalid configuration does not leave a previous one running
     *
     *  @param[in]  conf: Mutex and timing of the task
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK              Configuration stored
     *  @retval PlatformResult::InvalidParams   conf.mutex is NULL
     */
    PlatformResult ApplyConfig(const ezTaskConfiguration_t &conf)
    {
        is_configured = false;
        is_initialized = false;
        if(conf.mutex == nullptr)
        {
            return PlatformResult::InvalidParams;
        }

        this->config = conf;

        /* resolve the defaults once, so the users of config never have to check for 0 */
        if(this->config.lock_timeout_ms == 0)
        {
            this->config.lock_timeout_ms = EZ_DEFAULT_LOCK_TIMEOUT_MS;
        }
        if(this->config.task_delay_ms == 0)
        {
            this->config.task_delay_ms = EZ_DEFAULT_TASK_DELAY_MS;
        }
        return PlatformResult::OK;
    }

    volatile bool is_initialized = false;   /**< Set by the derived class when it is ready to run */
    volatile bool is_configured = false;    /**< Set when the configuration was accepted */
    ezEventListener_t event_listener;       /**< Listener of the task on the platform event bus */
    ezTaskConfiguration_t config{};         /**< Stored configuration, defaults already resolved */
};

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
