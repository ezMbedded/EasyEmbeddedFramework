/** @file   sensor_task.hpp
 *  @brief  Sensor task of the pi-pico sample
 *
 *  @details Sample application code, not part of the framework. It builds on the
 *  platform layer (ez_platform.hpp, ez_task.hpp), so it needs EZ_PLATFORM set to 1.
 */
#pragma once

#include "ez_task.hpp"
#include "ez_platform.hpp"

namespace ez { namespace application {

using namespace ez::platform;

/** @brief Sample sensor task that publishes a temperature reading on the platform
 *
 *  @details Derives from platform::Task and is configured with Setup() from the base class.
 *  Init() registers a listener on the platform event bus; that listener ignores every event.
 *  Each call of TaskEntry() writes a random temperature in [-20, 40) into
 *  DATA_POINT_TEMPERATURE and then sends EVENT_DATA_POINT_CHANGED. No sensor hardware is read.
 *
 *  Only one instance is supported: TaskEntry() uses a file-scope pointer that Init() sets.
 */
class SensorTask : public platform::Task
{
public:
    /** @brief Create the task
     *  @details Does nothing beyond the default member initialization. Call Setup() and Init() before use.
     */
    SensorTask() noexcept = default;

    /** @brief Default destructor, releases nothing */
    ~SensorTask() = default;

    /** @brief Register the task listener on the platform event bus
     *  @details Creates the event listener and registers it with Platform::RegisterListener().
     *           On success the task is initialized, the running flag is set and the task
     *           becomes the target of TaskEntry().
     *
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Task initialized
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Failed              The listener could not be created, or
     *                                              Platform::RegisterListener() returned an error
     *                                              (platform not set up, lock timeout, or the
     *                                              listener is already registered)
     *
     *  @pre Setup() succeeded, and Platform::instance().Setup() succeeded
     *  @post On OK the task is initialized and TaskEntry() can run
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see TaskEntry
     *
     *  \b Example
     *  @code
     *  if(sensor.Setup(sensor_config) != PlatformResult::OK
     *      || sensor.Init() != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    PlatformResult Init(void);

    /** @brief Clear the running flag
     *  @details Only sets the flag to false. Neither TaskEntry() nor Run() reads the flag in this
     *           implementation, so the sensor readings continue after Stop().
     *
     *  @warning Do not rely on Stop() to pause the sensor readings.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Start
     */
    void Stop(void);

    /** @brief Set the running flag
     *  @details Only sets the flag to true. Init() also sets it to true. The flag is not read
     *           by TaskEntry() or Run(), see Stop().
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Stop
     */
    void Start(void);

    /** @brief Make one sensor reading and publish it, called by the RTOS task that owns the sensor
     *  @details Each call does one cycle. It draws a random float in [-20, 40), stores it in
     *           DATA_POINT_TEMPERATURE with Platform::Set() and, only if that succeeded, sends
     *           EVENT_DATA_POINT_CHANGED with the index of the data point. The call returns after
     *           one cycle, so the owner calls it periodically. The context argument is not used.
     *           When the task is not configured or not initialized, it logs an error and returns.
     *           It waits for the platform mutexes, up to the 100 ms timeout of Platform::Set()
     *           and the lock timeout of Platform::SendEvent().
     *
     *  @param[in]  context: User context, not used
     *
     *  @pre Init() succeeded, and Platform::instance().Setup() succeeded with a data model that
     *       has DATA_POINT_TEMPERATURE of sizeof(float) bytes
     *  @post If Platform::Set() succeeded, EVENT_DATA_POINT_CHANGED is sent; otherwise no event is sent
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Init
     *
     *  \b Example
     *  @code
     *  void sensor_task(void *arg)
     *  {
     *      while(1)
     *      {
     *          SensorTask::TaskEntry(nullptr);
     *          vTaskDelay(pdMS_TO_TICKS(3000));
     *      }
     *  }
     *  @endcode
     */
    static void TaskEntry(void *context);

protected:
    void Run(void *context);

private:
    bool running = true;
};

}}