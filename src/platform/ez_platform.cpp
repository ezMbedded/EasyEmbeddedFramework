/*****************************************************************************
* Filename:         ez_platform.cpp
* Author:           Hai Nguyen
* Original Date:    30.10.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_platform.cpp
 *  @author Hai Nguyen
 *  @date   30.10.2025
 *  @brief  Configuration task, which configures the platform
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_platform.hpp"

#if (EZ_PLATFORM == 1)

#include <cstring>

#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_PLATFORM_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_platform"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;

void Platform::Run()
{
    if(IsInitialized() == false || this->is_running == false)
    {
        return;
    }

    ezSTATUS status = ezSUCCESS;
    {
        /* The mutex must be recursive: listeners may call SendEvent()/RegisterListener() */
        LockGuard lock;
        if(lock.Locked() == false)
        {
            return;
        }
        status = ezEventBus_Run(&this->event_bus);
    }

    if(status != ezSUCCESS)
    {
        EZWARNING("ezEventBus_Run() failed, error code = %d", status);
    }
}

void Platform::Stop()
{
    EZDEBUG("Stop()");
    this->is_running = false;
}

void Platform::Start(void)
{
    EZDEBUG("Start()");
    this->is_running = true;
}

PlatformResult Platform::RegisterListener(ezEventListener_t *listener)
{
    if(IsInitialized() == false)
    {
        return PlatformResult::TaskNotConfigured;
    }

    LockGuard lock;
    if(lock.Locked() == false)
    {
        return PlatformResult::Timeout;
    }

    if(ezEventBus_Listen(&instance().event_bus, listener) == ezSUCCESS)
    {
        return PlatformResult::OK;
    }
    return PlatformResult::Failed;
}

PlatformResult Platform::UnregisterListener(ezEventListener_t *listener)
{
    if(IsInitialized() == false)
    {
        return PlatformResult::TaskNotConfigured;
    }

    LockGuard lock;
    if(lock.Locked() == false)
    {
        return PlatformResult::Timeout;
    }

    if(ezEventBus_Unlisten(&instance().event_bus, listener) == ezSUCCESS)
    {
        return PlatformResult::OK;
    }
    return PlatformResult::Failed;
}

PlatformResult Platform::SendEvent(uint32_t event_code,
    const void *event_data,
    size_t event_data_size)
{
    if(IsInitialized() == false)
    {
        return PlatformResult::TaskNotConfigured;
    }

    if(event_data == nullptr && event_data_size > 0)
    {
        return PlatformResult::InvalidParams;
    }

    PlatformResult result = PlatformResult::Failed;
    {
        LockGuard lock;
        if(lock.Locked() == false)
        {
            return PlatformResult::Timeout;
        }

        if(ezEventBus_SendEvent(&instance().event_bus,
            event_code,
            event_data,
            event_data_size) == ezSUCCESS)
        {
            result = PlatformResult::OK;
        }
    }

    /* wake the platform task after the lock is released, so it does not block on it */
    if(result == PlatformResult::OK && Wake() == false)
    {
        EZWARNING("cannot wake the platform task");
    }
    return result;
}

PlatformResult Platform::Setup(const ezPlatformConfiguration_t &conf)
{
    /* an invalid or failed setup must not leave a previous valid one running */
    Invalidate();
    this->datamodel_created = false;

    const ezPlatformDataModelConfig_t &dm = conf.data_model;
    const bool has_data_model = (dm.data_points != nullptr)
        || (dm.num_of_data_points != 0)
        || (dm.buffer != nullptr)
        || (dm.buffer_size != 0);

    /* every lock has its own purpose, sharing a mutex would deadlock */
    if(conf.task_config.mutex == nullptr
        || conf.datamodel_mutex == nullptr
        || conf.task_config.wake_signal == nullptr
        || conf.task_config.mutex == conf.datamodel_mutex)
    {
        EZERROR("invalid sync objects: mutexes and signal must be provided, the mutexes must be different");
        return PlatformResult::InvalidParams;
    }

    if(conf.event_bus.buffer == nullptr
        || conf.event_bus.size == 0
        || conf.event_bus.size > UINT32_MAX)
    {
        EZERROR("invalid event bus buffer");
        return PlatformResult::InvalidParams;
    }

    if(has_data_model == true
        && (dm.data_points == nullptr
            || dm.num_of_data_points == 0
            || dm.buffer == nullptr
            || dm.buffer_size == 0))
    {
        EZERROR("data model is only partly provided");
        return PlatformResult::InvalidParams;
    }

    /* Init() stores the task configuration and calls OnInit(), which creates the bus and the data model.
     * The platform does not listen to its own bus, so there is no event callback */
    this->platform_config = conf;
    const PlatformResult result = Init(conf.task_config, nullptr);
    if(result == PlatformResult::OK)
    {
        EZDEBUG("platform setup ok");
    }
    return result;
}

PlatformResult Platform::OnInit()
{
    const ezPlatformDataModelConfig_t &dm = this->platform_config.data_model;
    const bool has_data_model = (dm.data_points != nullptr);

    if(ezEventBus_CreateBus(&this->event_bus,
        this->platform_config.event_bus.buffer,
        (uint32_t)this->platform_config.event_bus.size) != ezSUCCESS)
    {
        EZERROR("create platform event bus failed");
        return PlatformResult::EventBusNotCreated;
    }

    if(has_data_model == true)
    {
        if(ezDataModel_Initialize(&this->dataModel,
            dm.data_points, dm.num_of_data_points,
            dm.buffer, dm.buffer_size) == false)
        {
            EZERROR("create platform data model failed");
            return PlatformResult::DataModelNotCreated;
        }
        this->datamodel_created = true;
    }
    return PlatformResult::OK;
}

Mutex *Platform::DataModelMutex()
{
    return instance().platform_config.datamodel_mutex;
}

PlatformResult Platform::TakeDataModel(uint32_t timeout_ms)
{
    Mutex *mutex = DataModelMutex();
    if(IsInitialized() == false || mutex == nullptr)
    {
        return PlatformResult::TaskNotConfigured;
    }

    if(mutex->Lock(timeout_ms) == false)
    {
        return PlatformResult::Timeout;
    }
    return PlatformResult::OK;
}

PlatformResult Platform::GiveDataModel()
{
    Mutex *mutex = DataModelMutex();
    if(IsInitialized() == false || mutex == nullptr)
    {
        return PlatformResult::TaskNotConfigured;
    }

    if(mutex->Unlock() == false)
    {
        return PlatformResult::Failed;
    }
    return PlatformResult::OK;
}

PlatformResult Platform::SetDataPoint(uint32_t index, const void *data, size_t size)
{
    if(instance().datamodel_created == false)
    {
        return PlatformResult::DataModelNotCreated;
    }

    if(data == nullptr)
    {
        return PlatformResult::InvalidParams;
    }

    if(ezDataModel_SetDataPoint(&instance().dataModel, index, data, size) == ezSUCCESS)
    {
        return PlatformResult::OK;
    }
    return PlatformResult::Failed;
}

const void *Platform::GetDataPoint(uint32_t index, size_t *data_size)
{
    if(instance().datamodel_created == false)
    {
        return nullptr;
    }

    return ezDataModel_GetDataPoint(&instance().dataModel, index, data_size);
}

uint32_t Platform::GetFirstDirtyDataIndex()
{
    if(instance().datamodel_created == false)
    {
        return DATA_POINT_INVALID;
    }
    return ezDataModel_GetFirstDirty(&instance().dataModel);
}

PlatformResult Platform::ClearDirtyData(uint32_t index)
{
    if(instance().datamodel_created == false)
    {
        return PlatformResult::DataModelNotCreated;
    }

    ezDataModel_ClearDirtyFlags(&instance().dataModel, index);
    return PlatformResult::OK;
}

#endif /* EZ_PLATFORM */
