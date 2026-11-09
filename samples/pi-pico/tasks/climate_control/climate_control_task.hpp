/** @file   climate_control_task.hpp
 *  @brief  Climate control task of the pi-pico sample
 *
 *  @details Sample application code, not part of the framework. It builds on the
 *  platform layer (ez_platform.hpp, ez_task.hpp), so it needs EZ_PLATFORM set to 1.
 */
#pragma once

#include "ez_task.hpp"
#include "ez_platform.hpp"
#include "ez_state_machine_v2.h"

namespace ez { namespace application {

using namespace ez::platform;

/** @brief Keeps the temperature near the target by requesting a heater or a cooler
 *
 *  @details The task listens to the temperature data point on the platform event bus.
 *  A reading more than offset_temperature below the target moves the state machine to
 *  heating, a reading more than offset_temperature above it moves it to cooling. Entering
 *  or leaving a state sends EVENT_HEATER_START / EVENT_HEATER_STOP or
 *  EVENT_COOLER_START / EVENT_COOLER_STOP with Platform::SendEvent().
 *
 *  Only one instance is supported: the state machine and the state callbacks use
 *  file-scope statics. Setup() the task and the platform (Platform::Setup()) before Init().
 */
class ClimateControlTask : public platform::Task
{
public:
    /** @brief Create the task with a target temperature
     *  @details Init() resets the target to 20.0 before the state machine starts, so
     *           the value given here is only returned until Init() runs.
     *
     *  @param[in]  target_temperature: Target temperature, in the unit of the temperature data point
     */
    ClimateControlTask(float target_temperature)
    {
        this->target_temperature = target_temperature;
    }

    /** @brief Default destructor, releases nothing */
    ~ClimateControlTask() = default;

    /** @brief Register the task on the platform event bus and start the state machine
     *  @details Resets the target temperature to 20.0. Creates the event listener, registers
     *           it with Platform::RegisterListener(), then initializes the state machine and
     *           starts it in the idle state. The state machine uses static storage only.
     *
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Task initialized, state machine started
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Failed              The listener could not be created or registered
     *                                              (also on a lock timeout), or the state machine
     *                                              could not be initialized
     *
     *  @pre Setup() succeeded, and Platform::instance().Setup() succeeded
     *  @post On OK the task is initialized and the state machine is in the idle state
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see TaskEntry, Stop
     *
     *  \b Example
     *  @code
     *  if(climate_control.Setup(climate_control_config) != PlatformResult::OK
     *      || climate_control.Init() != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    PlatformResult Init(void);

    /** @brief Clear the running flag
     *  @details Only sets the flag to false. Run() does not read the flag in this
     *           implementation, so the state machine keeps running and Stop() has no effect on it.
     *
     *  @warning Do not rely on Stop() to pause the climate control.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Start
     */
    void Stop(void);

    /** @brief Set the running flag
     *  @details Only sets the flag to true. Init() also sets it to true. As with Stop(),
     *           Run() does not read the flag in this implementation.
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Stop
     */
    void Start(void);

    /** @brief Entry point of the task, call it periodically from an RTOS task
     *  @details Logs an error and returns if the task is not configured or not initialized.
     *           Otherwise it calls Run(), which processes the events queued by the temperature
     *           listener. A state change runs the entry or exit callback, and each callback
     *           calls Platform::SendEvent(), which waits for the event bus mutex.
     *           The function uses the static pointer set by Init() without a NULL check.
     *
     *  @param[in]  context: Not used by this task
     *
     *  @pre Init() was called
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Init
     *
     *  \b Example
     *  @code
     *  while(1)
     *  {
     *      ClimateControlTask::TaskEntry(nullptr);
     *      vTaskDelay(pdMS_TO_TICKS(100));
     *  }
     *  @endcode
     */
    static void TaskEntry(void *context);

    /**< Half width of the band around the target where no state change is requested, same unit as the temperature. Fixed at 0.5 */
    const float offset_temperature = 0.5f;

    /** @brief Get the target temperature
     *  @return     Target temperature, set by the constructor or reset to 20.0 by Init()
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see SetCurrentTemperature
     */
    float GetTargetTemperature()
    {
        return this->target_temperature;
    }

    /** @brief Store the latest temperature reading
     *  @details A plain store without a lock. The sample calls it from the platform event
     *           listener, and GetCurrentTemperature() is read from the state callbacks run by
     *           TaskEntry().
     *
     *  @param[in]  temperature: Latest temperature reading, in the unit of the temperature data point
     *
     *  @warning Not protected by a lock, see details.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see GetCurrentTemperature
     */
    void SetCurrentTemperature(float temperature)
    {
        this->current_temperature = temperature;
    }

    /** @brief Get the latest temperature stored by SetCurrentTemperature()
     *  @details Returns 0.0 until a value is set. Init() does not reset it.
     *
     *  @return     Latest stored temperature, 0.0 if none was set
     *  @warning Not protected by a lock, see SetCurrentTemperature()
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see SetCurrentTemperature
     */
    float GetCurrentTemperature()
    {
        return this->current_temperature;
    }

protected:
    void Run(void *context);

private:
    bool running = true;
    float target_temperature;
    float current_temperature = 0.0f;
};

}}