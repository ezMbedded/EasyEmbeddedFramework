/** @file   ez_platform.hpp
 *  @author Hai Nguyen
 *  @date   29.10.2026
 *  @brief  Platform singleton: event bus, data model and the platform task in one object
 *
 *  @details Only available when EZ_PLATFORM is 1. The platform owns one event bus and
 *  one optional data model. All memory (event queue, data point table and values) and
 *  all synchronization objects (two mutexes, one wake signal) are supplied by the
 *  application through Platform::Setup(); the platform does not allocate.
 *  Call Setup() first, register the listeners, then run Platform::Entry() in an RTOS task.
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

/** @brief Memory of the platform event bus
 *  @details Required. The memory is owned by the caller and is not copied, see Platform::Setup()
 */
typedef struct
{
    uint8_t *buffer;                    /**< Buffer of the event queue, must not be NULL */
    size_t size;                        /**< Size of buffer in bytes, must not be 0 and not larger than UINT32_MAX */
}ezPlatformEventBusConfig_t;

/** @brief Memory of the platform data model
 *  @details Optional. Leave all four fields 0/NULL for no data model. If one field is set, all four must be set.
 *           The table must be sorted by strictly increasing index, see ezDataModel_Initialize().
 *           The memory is owned by the caller and is not copied.
 */
typedef struct
{
    ezDataPoint_t *data_points;         /**< Table of the data points */
    size_t num_of_data_points;          /**< Number of data points in the table, must not be 0 when a data model is used */
    uint8_t *buffer;                    /**< Buffer that stores the values of the data points, zeroed by Setup() */
    size_t buffer_size;                 /**< Size of buffer in bytes, must not be 0 when a data model is used */
}ezPlatformDataModelConfig_t;

/** @brief Everything the platform needs, see Platform::Setup()
 *  @details The struct is copied by Setup(). The mutexes, the signal and the memory it points to are not
 *           copied and must stay valid while the platform is used.
 */
typedef struct
{
    /** Required. Its fields:
     *  - mutex: the event bus mutex, must not be NULL and must be recursive (listeners run with it held
     *    and may call SendEvent()/RegisterListener()). Different from datamodel_mutex.
     *  - wake_signal: required, the platform task sleeps on it and SendEvent() notifies it.
     *  - delay: must be NULL. Setup() rejects it with InvalidParams.
     *  - lock_timeout_ms, task_delay_ms: 0 selects EZ_DEFAULT_LOCK_TIMEOUT_MS and EZ_DEFAULT_TASK_DELAY_MS.
     */
    ezTaskConfiguration_t task_config;
    Mutex *datamodel_mutex;             /**< Required. Protects the data model, see TakeDataModel(). Must differ from task_config.mutex */
    ezPlatformEventBusConfig_t event_bus;           /**< Required, see ezPlatformEventBusConfig_t */
    ezPlatformDataModelConfig_t data_model;         /**< Optional, see ezPlatformDataModelConfig_t */
}ezPlatformConfiguration_t;

/** @brief Singleton that bundles the event bus, the data model and the platform task
 *  @details Get the instance with Platform::instance(), which ezTask<Platform> provides. Entry() and
 *           Wake() are the inherited static members of ezTask<Platform>. Configure the platform with
 *           Setup(), not with the inherited ezTask::Init(): Setup() validates the memory and stores it
 *           for the creation of the event bus and the data model.
 *
 *  @see ezTask
 */
class Platform : public ezTask<Platform>
{
public:
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    /** @brief Configure the platform: creates the event bus and the data model. Call it first and once
     *  @details All or nothing. It fails with
     *           - InvalidParams: the task mutex, the data model mutex or the wake signal is NULL, the two
     *             mutexes are the same object, task_config.delay is set, the event bus buffer is NULL or
     *             its size is 0 or larger than UINT32_MAX, or the data model is only partly provided
     *             (one of its four fields is set and another one is not).
     *           - EventBusNotCreated: the event bus rejected its memory (ezEventBus_CreateBus() failed).
     *           - DataModelNotCreated: the data model rejected its memory (ezDataModel_Initialize() failed).
     *           A failed call leaves the platform unconfigured, so Entry() does not run it.
     *           Calling it again re-creates the event bus and the data model: the registered listeners
     *           are lost and the data model values are zeroed. It does not change the Stop()/Start() flag.
     *           Setup() takes no lock. Do not call it while the platform task is running.
     *           The mutexes, the signal and the memory are not copied, they are owned by the caller
     *           and must outlive the platform. The structure conf is copied.
     *           lock_timeout_ms and task_delay_ms equal to 0 are replaced by
     *           EZ_DEFAULT_LOCK_TIMEOUT_MS and EZ_DEFAULT_TASK_DELAY_MS.
     *
     *  @param[in]  conf: Memory, mutexes, wake signal and timing of the platform, see ezPlatformConfiguration_t
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Platform configured
     *  @retval PlatformResult::InvalidParams       Invalid mutex, signal or memory, see details
     *  @retval PlatformResult::EventBusNotCreated  The event bus rejected its memory
     *  @retval PlatformResult::DataModelNotCreated The data model rejected its memory
     *
     *  @pre The mutexes, the signal and the memory in conf must stay valid while the platform is used
     *  @post On OK, Entry() runs the platform and the listeners can be registered. On failure the platform is unconfigured
     *  @warning Calling it again loses the registered listeners.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Entry
     *
     *  \b Example
     *  @code
     *  ezPlatformConfiguration_t config = {};
     *  config.task_config.mutex = &task_mutex;
     *  config.task_config.wake_signal = &wake_signal;
     *  config.datamodel_mutex = &datamodel_mutex;
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
     *  @details Sets a flag. While it is cleared, Run() returns without delivering the queued events to the
     *           listeners. The events stay in the queue, and SendEvent() still queues new ones and wakes the
     *           platform task. Undone by Start(). Setup() does not change the flag.
     *  @see Start
     */
    void Stop(void);

    /** @brief Resume handling events after Stop(). The flag is set when the platform is created
     *  @details Setup() does not change the flag, so a platform stopped before Setup() stays stopped until Start().
     *  @see Stop
     */
    void Start(void);

    /** @brief Register a listener on the platform event bus
     *  @details Takes the event bus mutex of the task configuration for up to lock_timeout_ms.
     *           The listener is linked into the bus, so it must stay valid while it is registered.
     *           The callback runs in Run() while the event bus mutex is held.
     *
     *  @warning The callback must not call UnregisterListener() (see EVENT_CALLBACK in ez_event_bus.h).
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
     *           event bus queue. The platform task is woken up after the mutex is released, so it
     *           does not wait on it. The wake-up is tried only when the event was queued; if it
     *           fails, only a warning is logged and the result is still OK.
     *
     *  @param[in]  event_code: Code of the event
     *  @param[in]  event_data: Data of the event, may be NULL if event_data_size is 0
     *  @param[in]  event_data_size: Size of event_data in bytes
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Event queued, the platform task is woken up
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::InvalidParams       event_data is NULL but event_data_size is not 0
     *  @retval PlatformResult::Timeout             The mutex could not be taken in time
     *  @retval PlatformResult::Failed              The event bus rejected the event: no space in the queue,
     *                                              or the data is too big for it
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
     *  @details The data model functions below do NOT lock by themselves (except Get() and Set(), which
     *           lock for one access). The application decides when to protect the others: lock, use the
     *           data model, unlock. The mutex is taken for up to timeout_ms; 0 is passed to the mutex as
     *           it is, so the call does not wait. EZ_DEFAULT_LOCK_TIMEOUT_MS is only the default value.
     *           Do not call SendEvent() or other blocking functions while holding it.
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
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model (also when not configured)
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
     *  @details The data is copied into the data model and the data point is marked dirty.
     *           The data model does not lock, so the caller serializes the access when other tasks use it.
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
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Set
     */
    static PlatformResult SetDataPoint(uint32_t index,
        const void *data,
        size_t size);

    /** @brief Get a data point, NOT protected by the data model mutex, see TakeDataModel()
     *  @details The pointer references the data model buffer, no copy is made. The data is only
     *           stable while the data model is taken, because another task may call SetDataPoint() or Set().
     *           data_size is set to 0 when the index is unknown. Nothing is written when the data model is not created.
     *
     *  @warning Do not write through the pointer, use SetDataPoint() or Set()
     *
     *  @param[in]  index: Index of the data point
     *  @param[out] data_size: Receives the size of the data point in bytes. NULL makes the call return nullptr
     *  @return     Pointer to the data point, or nullptr if the data model is not created, the index is unknown or data_size is NULL
     *
     *  @pre Setup() succeeded with a data model; the caller holds the data model lock if other tasks use it
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Get
     */
    static const void* GetDataPoint(
        uint32_t index,
        size_t *data_size);

    /** @brief Get the index of the first dirty data point, NOT protected by the data model mutex
     *  @details The data points are searched in table order. A dirty point stays dirty until ClearDirtyData().
     *
     *  @return Index of the first dirty data point, or DATA_POINT_INVALID if none or data model not created
     *
     *  @isr_safe{no}
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
     *          save(index, data, size);    // application function, not part of the platform
     *          Platform::ClearDirtyData(index);
     *          index = Platform::GetFirstDirtyDataIndex();
     *      }
     *      Platform::GiveDataModel();
     *  }
     *  @endcode
     */
    static uint32_t GetFirstDirtyDataIndex();

    /** @brief Clear the dirty flag of a data point, NOT protected by the data model mutex
     *  @details The data model clears the flag of the index. An unknown index is ignored and the
     *           call still returns OK, because the data model reports no status.
     *
     *  @param[in]  index: Index of the data point
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Dirty flag cleared, or the index is unknown and nothing changed
     *  @retval PlatformResult::DataModelNotCreated Setup() did not create a data model
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see GetFirstDirtyDataIndex
     */
    static PlatformResult ClearDirtyData(uint32_t index);


private:
    friend class ezTask<Platform>;        /* provides the instance and calls Run() and OnInit() */

    /* The platform is configured with Setup(), which validates the configuration first. The inherited Init() would skip that */
    using ezTask<Platform>::Init;
    friend struct PlatformTestAccess;   /* test-only access to private members */

    Platform() noexcept = default;
    ~Platform() = default;

    /** @brief Take the event bus mutex and deliver the queued events to the listeners, called by Entry()
     *  @details Returns at once if the platform is not configured or Stop() was called. Otherwise it
     *           runs the event bus with the event bus mutex held, for up to lock_timeout_ms.
     */
    void Run();

    /** @brief Create the event bus and the data model from platform_config, called by Init() through ezTask<Platform>
     *  @details Setup() stores the configuration before Init() calls this. Returns EventBusNotCreated or
     *           DataModelNotCreated when the memory is rejected, see Setup().
     *  @return PlatformResult
     */
    PlatformResult OnInit();

    /** @brief Data model mutex from platform_config */
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
