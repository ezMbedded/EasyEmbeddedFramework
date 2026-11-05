/** @file   ez_platform.hpp
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Platform singleton: event bus, data model and the platform task in one object
 *
 *  @details Only available when EZ_PLATFORM is 1. The platform owns one event bus and
 *  one optional data model. All memory (event queue, data point table and values) and
 *  all synchronization objects (two mutexes, one wake signal) are supplied by the
 *  application through Platform::Setup(); the platform does not allocate.
 *  Call Setup() first, register the listeners, then run Platform::TaskEntry() in an RTOS task.
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

/** @brief Singleton that bundles the event bus, the data model and the platform task */
class Platform : public Task
{
public:
    /** @brief Get the one and only platform object
     *  @return Reference to the platform singleton
     */
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
     *           listeners are lost. Do not call it while the platform task is running.
     *           The memory in conf is owned by the caller and must outlive the platform.
     *           lock_timeout_ms and task_delay_ms equal to 0 are replaced by
     *           EZ_DEFAULT_LOCK_TIMEOUT_MS and EZ_DEFAULT_TASK_DELAY_MS.
     *
     *  @param[in]  conf: Memory, mutexes, signal and timing of the platform
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Platform configured
     *  @retval PlatformResult::InvalidParams       Invalid mutex, signal or memory, see details
     *  @retval PlatformResult::EventBusNotCreated  The event bus rejected its memory
     *  @retval PlatformResult::DataModelNotCreated The data model rejected its memory
     *
     *  @pre The mutexes and the signal in conf must stay valid while the platform is used
     *  @post On success Platform::TaskEntry() can run; on failure the platform is unconfigured
     *  @warning Calling it again loses the registered listeners.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see TaskEntry
     *
     *  \b Example
     *  @code
     *  ezPlatformConfiguration_t config = {};
     *  config.task_config.mutex = &task_mutex;
     *  config.datamodel_mutex = &datamodel_mutex;
     *  config.wake_signal = &wake_signal;
     *  config.event_bus.buffer = event_bus_buffer;
     *  config.event_bus.size = sizeof(event_bus_buffer);
     *  if(Platform::instance().Setup(config) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    PlatformResult Setup(const ezPlatformConfiguration_t &conf);

    /** @brief Stop handling events
     *  @details Sets a flag. While it is cleared, the platform task does not deliver the queued
     *           events to the listeners; SendEvent() still queues them. Undone by Start()
     *  @see Start
     */
    void Stop(void) override;

    /** @brief Resume handling events after Stop(). The platform is running by default
     *  @see Stop
     */
    void Start(void) override;

    /** @brief Register a listener on the platform event bus
     *  @details Takes the event bus mutex of the task configuration for up to lock_timeout_ms
     *
     *  @param[in]  listener: Listener created with ezEventBus_CreateListener(). The caller owns it
     *              and it must stay valid while registered
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Listener registered
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              Listener is NULL or already registered
     *
     *  @pre Setup() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see UnregisterListener
     *
     *  \b Example
     *  @code
     *  ezEventListener_t listener;
     *  if(ezEventBus_CreateListener(&listener, OnEvent) != ezSUCCESS
     *      || Platform::RegisterListener(&listener) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    static PlatformResult RegisterListener(ezEventListener_t *listener);

    /** @brief Remove a listener from the platform event bus
     *  @details Takes the event bus mutex of the task configuration for up to lock_timeout_ms
     *
     *  @param[in]  listener: Listener that was registered with RegisterListener()
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Listener removed
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              Listener is NULL or not registered
     *
     *  @pre Setup() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see RegisterListener
     */
    static PlatformResult UnregisterListener(ezEventListener_t *listener);

    /** @brief Queue an event for the listeners and wake up the platform task
     *  @details Not for interrupts, it waits for the mutex. The event data is copied into the
     *           event bus queue. The platform task is woken up only when the event was queued,
     *           after the mutex is released
     *
     *  @param[in]  event_code: Code of the event
     *  @param[in]  event_data: Data of the event, may be NULL if event_data_size is 0
     *  @param[in]  event_data_size: Size of event_data in bytes
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Event queued and the task woken up
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::InvalidParams       event_data is NULL but event_data_size is not 0
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              The event does not fit into the queue
     *
     *  @pre Setup() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *
     *  \b Example
     *  @code
     *  uint32_t index = 0;
     *  if(Platform::SendEvent(EVENT_TEMPERATURE_CHANGED, &index, sizeof(index)) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    static PlatformResult SendEvent(uint32_t event_code,
        const void *event_data,
        size_t event_data_size);

    /** @brief Lock the data model
     *  @details The data model functions below do NOT lock by themselves. The application decides
     *           when to protect them: lock, use the data model, unlock.
     *           Do not call SendEvent() or other blocking functions while holding it
     *
     *  @param[in]  timeout_ms: Max time to wait for the data model mutex in milliseconds
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Data model locked
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *
     *  @pre Setup() succeeded
     *  @post On OK the caller must call GiveDataModel()
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see GiveDataModel
     */
    static PlatformResult TakeDataModel(uint32_t timeout_ms = EZ_DEFAULT_LOCK_TIMEOUT_MS);

    /** @brief Unlock the data model, only after a successful TakeDataModel()
     *
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Data model unlocked
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Failed              The mutex could not be released
     *
     *  @pre TakeDataModel() returned OK
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see TakeDataModel
     */
    static PlatformResult GiveDataModel();

    /** @brief Read a data point into value, protected by the data model mutex
     *  @details Do not call it while holding the data model lock (TakeDataModel()).
     *           The value is copied with memcpy, T must be trivially copyable and its size must
     *           be the size of the data point
     *
     *  @tparam     T: Type of the value
     *  @param[in]  index: Index of the data point
     *  @param[out] value: Receives the value of the data point
     *  @param[in]  timeout_ms: Max time to wait for the data model mutex in milliseconds
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Value read
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              Unknown index
     *  @retval PlatformResult::InvalidParams       Size of T is not the size of the data point
     *
     *  @pre Setup() succeeded with a data model
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Set
     *
     *  \b Example
     *  @code
     *  float temperature = 0.0f;
     *  if(Platform::Get(DATA_POINT_TEMPERATURE, temperature) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
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
     *           The value is copied into the data model and the data point is marked dirty.
     *           T must be trivially copyable and its size must be the size of the data point
     *
     *  @tparam     T: Type of the value
     *  @param[in]  index: Index of the data point
     *  @param[in]  value: Value to store
     *  @param[in]  timeout_ms: Max time to wait for the data model mutex in milliseconds
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Value stored
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              Unknown index or wrong size
     *
     *  @pre Setup() succeeded with a data model
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Get
     *
     *  \b Example
     *  @code
     *  float temperature = 21.5f;
     *  if(Platform::Set(DATA_POINT_TEMPERATURE, temperature) != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
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

    /** @brief Write a data point, NOT protected by the data model mutex, see TakeDataModel()
     *  @details The data is copied into the data model and the data point is marked dirty
     *
     *  @param[in]  index: Index of the data point
     *  @param[in]  data: Value to copy
     *  @param[in]  size: Size of data in bytes, must be the size of the data point
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Value stored
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model
     *  @retval PlatformResult::InvalidParams       data is NULL
     *  @retval PlatformResult::Failed              Unknown index or wrong size
     *
     *  @pre Setup() succeeded with a data model; the caller holds the data model lock if other tasks use it
     *  @blocking{no}
     *  @see Set
     */
    static PlatformResult SetDataPoint(uint32_t index,
        const void *data,
        size_t size);

    /** @brief Get a data point, NOT protected by the data model mutex, see TakeDataModel()
     *  @details The pointer references the data model buffer, no copy is made. It stays valid
     *           and unchanged only while the data model is taken
     *
     *  @param[in]  index: Index of the data point
     *  @param[out] data_size: Receives the size of the data point in bytes
     *  @return     Pointer to the data point, or nullptr if the data model is not created or the index is unknown
     *
     *  @pre Setup() succeeded with a data model; the caller holds the data model lock if other tasks use it
     *  @blocking{no}
     *  @see Get
     */
    static const void* GetDataPoint(
        uint32_t index,
        size_t *data_size);

    /** @brief Get the index of the first dirty data point, NOT protected by the data model mutex
     *  @return Index of the first dirty data point, or DATA_POINT_INVALID if none or data model not created
     *  @blocking{no}
     *  @see ClearDirtyData
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

    /** @brief Clear the dirty flag of a data point, NOT protected by the data model mutex
     *
     *  @param[in]  index: Index of the data point
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Dirty flag cleared (the result of the data model call is not checked)
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model
     *
     *  @blocking{no}
     *  @see GetFirstDirtyDataIndex
     */
    static PlatformResult ClearDirtyData(uint32_t index);

    /** @brief Entry point of the platform task, hand it to the RTOS
     *  @details Create the RTOS task only after Setup() succeeded, otherwise it returns at once.
     *           It never returns after that: it handles the events, then sleeps until SendEvent()
     *           wakes it up or the task delay expires
     *
     *  @param[in]  context: Passed on to the task, not used by the platform
     *
     *  @pre Setup() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Setup
     *
     *  \b Example
     *  @code
     *  std::thread platform_task(Platform::TaskEntry, nullptr);
     *  @endcode
     */
    static void TaskEntry(void *context);


protected:
    /** @brief Take the event bus mutex and deliver the queued events to the listeners, called by TaskEntry() */
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
