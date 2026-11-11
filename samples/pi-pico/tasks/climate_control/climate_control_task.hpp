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
 *  @details An event-driven task. OnEvent() listens to the temperature data point on the platform
 *  event bus (it runs in the platform task), posts an event to the state machine and wakes this
 *  task with Wake(). Run() lets the state machine handle the posted events. A reading more than
 *  offset_temperature below the target moves the state machine to heating, a reading more than
 *  offset_temperature above it moves it to cooling. Entering or leaving a state sends
 *  EVENT_HEATER_START / EVENT_HEATER_STOP or EVENT_COOLER_START / EVENT_COOLER_STOP with
 *  Platform::SendEvent().
 *
 *  Only one instance exists, see ClimateControlTask::instance(). Start it with
 *  ClimateControlTask::Init(config, ClimateControlTask::OnEvent), after Platform::Init(),
 *  and give ClimateControlTask::Entry to the RTOS. The target temperature starts at 20.0.
 */
class ClimateControlTask : public platform::ezTask<ClimateControlTask>
{
    friend class platform::ezTask<ClimateControlTask>;
public:
    /**< Half width of the band around the target where no state change is requested, same unit as the temperature. Fixed at 0.5 */
    static constexpr float offset_temperature = 0.5f;

    /** @brief Listener callback, give it to ClimateControlTask::Init(). Runs in the platform task
     *  @param[in]  event_code: Code of the platform event
     *  @param[in]  data: Data of the event
     *  @param[in]  data_size: Size of data in bytes
     *  @return     0
     */
    static int OnEvent(uint32_t event_code, const void *data, size_t data_size);

    /** @brief Get the target temperature
     *  @return     Target temperature, in the unit of the temperature data point
     */
    float GetTargetTemperature()
    {
        return this->target_temperature;
    }

    /** @brief Store the latest temperature reading
     *  @details A plain store without a lock. It is called from OnEvent() and the value is read
     *           from the state callbacks run by Run().
     *  @param[in]  temperature: Latest temperature reading
     *  @warning Not protected by a lock, see details.
     */
    void SetCurrentTemperature(float temperature)
    {
        this->current_temperature = temperature;
    }

    /** @brief Get the latest temperature stored by SetCurrentTemperature()
     *  @return     Latest stored temperature, 0.0 if none was set
     *  @warning Not protected by a lock, see SetCurrentTemperature()
     */
    float GetCurrentTemperature()
    {
        return this->current_temperature;
    }

private:
    ClimateControlTask() = default;
    ~ClimateControlTask() = default;

    /** @brief Reset the target and start the state machine in the idle state, called by Init() */
    PlatformResult OnInit();

    /** @brief One pass: the state machine handles the posted events */
    void Run();

    float target_temperature = 20.0f;
    float current_temperature = 0.0f;
};

}}
