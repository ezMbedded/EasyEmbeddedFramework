/*****************************************************************************
* Filename:         ez_platform.hpp
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

/** @file   ez_platform.hpp
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Configuration task, which configures the platform
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

#include <cstring>
#include <type_traits>

#include "ez_task.hpp"
#include "ez_event_bus.h"
#include "ez_data_model.h"

namespace ez { 
namespace platform{

/** @brief Memory of the platform event bus */
typedef struct
{
    uint8_t *buffer;                    /**< Buffer of the event queue */
    size_t size;                        /**< Size of buffer in bytes */
}ezPlatformEventBusConfig_t;

/** @brief Memory of the platform data model */
typedef struct
{
    ezDataPoint_t *data_points;         /**< Table of the data points */
    size_t num_of_data_points;          /**< Number of data points in the table */
    uint8_t *buffer;                    /**< Buffer that stores the values of the data points */
    size_t buffer_size;                 /**< Size of buffer in bytes */
}ezPlatformDataModelConfig_t;

/** @brief Everything the platform needs, see Platform::Setup() */
typedef struct
{
    ezTaskConfiguration_t task_config;  /**< Recursive mutex of the event bus, lock timeout and task delay */
    Mutex *datamodel_mutex;             /**< Required. Protects the data model, see TakeDataModel(). Different from the task mutex */
    Signal *wake_signal;                /**< Required. The platform task waits on it, SendEvent() notifies it */
    ezPlatformEventBusConfig_t event_bus;           /**< Required */
    ezPlatformDataModelConfig_t data_model;         /**< Optional: leave all fields 0/NULL for no data model */
}ezPlatformConfiguration_t;

class Platform : public Task
{
public:
    static Platform& instance() noexcept { return s_instance; }

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    /** @brief Configure the platform: creates the event bus and the data model, call it first and once
     *  @details All or nothing. It fails with
     *           - InvalidParams: a mutex or the signal is NULL, both mutexes are the same object,
     *             the event bus memory is missing, or the data model memory is only partly provided.
     *           - EventBusNotCreated or DataModelNotCreated: the memory is rejected by the event bus
     *             or the data model.
     *           A failed call leaves the platform unconfigured, so it does not run.
     *           Calling it again re-creates the event bus and the data model, so the registered
     *           listeners are lost. Do not call it while the platform task is running
     */
    PlatformResult Setup(const ezPlatformConfiguration_t &conf);

    void Stop(void) override;
    void Start(void) override;

    /** @return OK, TaskNotConfigured, Timeout or Failed (the listener is already registered) */
    static PlatformResult RegisterListener(ezEventListener_t *listener);

    /** @return OK, TaskNotConfigured, Timeout or Failed (the listener is not registered) */
    static PlatformResult UnregisterListener(ezEventListener_t *listener);

    /** @brief Queue an event for the listeners and wake up the platform task
     *  @details Not for interrupts, it waits for the mutex
     *  @return OK, TaskNotConfigured, InvalidParams (data is NULL but size is not 0),
     *          Timeout or Failed (the event does not fit into the queue)
     */
    static PlatformResult SendEvent(uint32_t event_code,
        const void *event_data,
        size_t event_data_size);

    /** @brief Lock the data model
     *  @details The data model functions below do NOT lock by themselves. The application decides
     *           when to protect them: lock, use the data model, unlock.
     *           Do not call SendEvent() or other blocking functions while holding it
     *  @param timeout_ms   Max time to wait
     *  @return OK, TaskNotConfigured or Timeout
     */
    static PlatformResult TakeDataModel(uint32_t timeout_ms = EZ_DEFAULT_LOCK_TIMEOUT_MS);

    /** @brief Unlock the data model, only after a successful TakeDataModel()
     *  @return OK, TaskNotConfigured or Failed
     */
    static PlatformResult GiveDataModel();

    /** @brief Read a data point into value, protected by the data model mutex
     *  @details Do not call it while holding the data model lock (TakeDataModel()).
     *  @return OK, DataModelNotCreated, Timeout, Failed (unknown index) or InvalidParams (size of T is not the size of the data point)
     */
    template <typename T>
    static PlatformResult Get(uint32_t index, T &value, uint32_t timeout_ms = EZ_DEFAULT_LOCK_TIMEOUT_MS)
    {
        static_assert(std::is_trivially_copyable<T>::value, "data point type must be trivially copyable");

        if(instance().datamodel_created == false)
        {
            return PlatformResult::DataModelNotCreated;
        }

        PlatformResult result = TakeDataModel(timeout_ms);
        if(result != PlatformResult::OK)
        {
            return result;
        }

        size_t size = 0;
        const void *data = GetDataPoint(index, &size);
        if(data == nullptr)
        {
            result = PlatformResult::Failed;
        }
        else if(size != sizeof(T))
        {
            result = PlatformResult::InvalidParams;
        }
        else
        {
            std::memcpy(&value, data, sizeof(T));
        }
        GiveDataModel();
        return result;
    }

    /** @brief Write a data point from value, protected by the data model mutex
     *  @details Do not call it while holding the data model lock (TakeDataModel()).
     *  @return OK, DataModelNotCreated, Timeout or Failed (unknown index or wrong size)
     */
    template <typename T>
    static PlatformResult Set(uint32_t index, const T &value, uint32_t timeout_ms = EZ_DEFAULT_LOCK_TIMEOUT_MS)
    {
        static_assert(std::is_trivially_copyable<T>::value, "data point type must be trivially copyable");

        if(instance().datamodel_created == false)
        {
            return PlatformResult::DataModelNotCreated;
        }

        PlatformResult result = TakeDataModel(timeout_ms);
        if(result != PlatformResult::OK)
        {
            return result;
        }

        result = SetDataPoint(index, &value, sizeof(T));
        GiveDataModel();
        return result;
    }

    /* The data model functions below are not protected, see TakeDataModel() */
    static PlatformResult SetDataPoint(uint32_t index,
        const void *data,
        size_t size);

    /** @brief Get a data point. The pointer stays valid and unchanged only while the data model is taken
     *  @return pointer to the data point, or nullptr if the data model is not created or the index is unknown
     */
    static const void* GetDataPoint(
        uint32_t index,
        size_t *data_size);

    /** @return Index of the first dirty data point, or DATA_POINT_INVALID if none or data model not created
     *  @note   Do the whole sequence inside one TakeDataModel()/GiveDataModel(), or a data point set in between
     *          loses its dirty flag without being saved:
     *  @code
     *  if(Platform::TakeDataModel(50) == PlatformResult::OK)
     *  {
     *      uint32_t index = Platform::GetFirstDirtyDataIndex();
     *      while(index != DATA_POINT_INVALID)
     *      {
     *          size_t size = 0;
     *          const void *data = Platform::GetDataPoint(index, &size);
     *          save(index, data, size);
     *          Platform::ClearDirtyData(index);
     *          index = Platform::GetFirstDirtyDataIndex();
     *      }
     *      Platform::GiveDataModel();
     *  }
     *  @endcode
     */
    static uint32_t GetFirstDirtyDataIndex();

    static PlatformResult ClearDirtyData(uint32_t index);

    /** @brief Entry point of the platform task, hand it to the RTOS
     *  @details Create the RTOS task only after Setup() succeeded, otherwise it returns at once.
     *           It never returns after that: it handles the events, then sleeps until SendEvent()
     *           wakes it up or the task delay expires
     */
    static void TaskEntry(void *context);


protected:
    void Run(void *context) override;
    Platform() noexcept = default;
    ~Platform() = default;

private:
    friend struct PlatformTestAccess;   /* test-only access to private/protected members */

    static Platform s_instance;

    static Mutex *DataModelMutex();

    volatile bool datamodel_created = false;
    volatile bool is_running = true;

    ezEventBus_t event_bus;
    ezDataModel_t dataModel;
    ezPlatformConfiguration_t platform_config{};
};


} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
