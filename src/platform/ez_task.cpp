/*****************************************************************************
* Filename:         ez_task.cpp
* Author:           Hai Nguyen
* Original Date:    10.11.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_task.cpp
 *  @author Hai Nguyen
 *  @date   10.11.2026
 *  @brief  Part of the platform task that does not depend on the derived class
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_task.hpp"

#if (EZ_PLATFORM == 1)

#include "ez_platform.hpp"
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_PLATFORM_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_task"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;

PlatformResult ezTaskCore::Init(const ezTaskConfiguration_t &conf,
    EVENT_CALLBACK on_event,
    PlatformResult (*on_init)())
{
    /* a failed call must not leave a previous valid initialization running */
    this->state = ezTaskState::Uninitialized;

    /* exactly one way to sleep */
    if((conf.wake_signal == nullptr) == (conf.delay == nullptr))
    {
        EZERROR("set either wake_signal or delay");
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

    if(on_init != nullptr)
    {
        const PlatformResult result = on_init();
        if(result != PlatformResult::OK)
        {
            return result;
        }
    }

    /* the listener is registered last, so no event reaches a half initialized task */
    if(on_event != nullptr)
    {
        if(ezEventBus_CreateListener(&this->listener, on_event) != ezSUCCESS)
        {
            EZERROR("cannot create the event listener");
            return PlatformResult::EventListenerNotCreated;
        }

        const PlatformResult result = Platform::RegisterListener(&this->listener);
        if(result != PlatformResult::OK)
        {
            EZERROR("cannot register the event listener");
            return result;
        }
    }

    this->state = ezTaskState::Initialized;
    return PlatformResult::OK;
}

bool ezTaskCore::BeginEntry() const
{
    if(this->state != ezTaskState::Initialized)
    {
        EZERROR("task is not initialized, call Init() before creating the RTOS task");
        return false;
    }
    return true;
}

void ezTaskCore::Sleep()
{
    if(this->config.wake_signal != nullptr)
    {
        /* returns early when woken, the timeout is only the fallback */
        if(this->config.wake_signal->Wait(this->config.task_delay_ms) == false)
        {
            EZTRACE("sleep timed out");
        }
    }
    else if(this->config.delay != nullptr)
    {
        this->config.delay->Sleep(this->config.task_delay_ms);
    }
}

bool ezTaskCore::Wake()
{
    if(this->config.wake_signal == nullptr)
    {
        return false;
    }
    return this->config.wake_signal->Notify();
}

bool ezTaskCore::Lock()
{
    if(this->config.mutex == nullptr)
    {
        return true;
    }
    return this->config.mutex->Lock(this->config.lock_timeout_ms);
}

void ezTaskCore::Unlock()
{
    if(this->config.mutex == nullptr)
    {
        return;
    }

    if(this->config.mutex->Unlock() == false)
    {
        EZERROR("cannot release the task mutex");
    }
}

#endif /* EZ_PLATFORM */
