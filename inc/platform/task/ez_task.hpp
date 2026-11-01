/*****************************************************************************
* Filename:         ez_task.h
* Author:           Hai Nguyen
* Original Date:    29.10.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_task.h
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Common interface for task
 *
 *  @details None
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

enum class PlatformResult : uint8_t
{
    OK,
    Failed,
    InvalidParams,
    EventBusNotCreated,
    EventListenerNotCreated,
    DataModelNotCreated,
    TaskNotConfigured,
    Timeout,    /**< Could not get the lock in time, can be retried */
    NoData,     /**< Nothing to report, e.g. no dirty data point pending */
};

constexpr uint32_t EZ_DEFAULT_LOCK_TIMEOUT_MS = 500;   /**< Used when lock_timeout_ms is 0 */
constexpr uint32_t EZ_DEFAULT_TASK_DELAY_MS   = 1000;  /**< Used when task_delay_ms is 0 */

typedef struct {
    Mutex *mutex;                   /**< Must be recursive: event listeners may call back into the platform */
    uint32_t lock_timeout_ms;       /**< Max time to wait for a mutex, 0 = EZ_DEFAULT_LOCK_TIMEOUT_MS */
    uint32_t task_delay_ms;         /**< Max time the task sleeps while waiting for an event, 0 = EZ_DEFAULT_TASK_DELAY_MS */
}ezTaskConfiguration_t;

class Task
{
public:
    virtual ~Task() = default;
    virtual void Stop() = 0;
    virtual void Start() = 0;

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
    virtual void Run(void *context) = 0;

    /** @brief Validate and store the configuration without marking the task as configured
     *  @details For the tasks that have more to set up. A failed call also marks the task as
     *           not configured, so an invalid configuration does not leave a previous one running
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

    volatile bool is_initialized = false;
    volatile bool is_configured = false;
    ezEventListener_t event_listener;
    ezTaskConfiguration_t config{};
};

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
