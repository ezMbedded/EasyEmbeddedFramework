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
 *  @details A periodic task: it sleeps with a Delay (task_delay_ms of its configuration) and
 *  needs no mutex and no event listener. Each pass of Run() writes a random temperature in
 *  [-20, 40) into the DATA_POINT_TEMPERATURE data point and sends EVENT_DATA_POINT_CHANGED.
 *
 *  Only one instance exists, see SensorTask::instance(). Start it with
 *  SensorTask::Init(config) and give SensorTask::Entry to the RTOS.
 */
class SensorTask : public platform::ezTask<SensorTask>
{
    friend class platform::ezTask<SensorTask>;

    SensorTask() = default;
    ~SensorTask() = default;

    /** @brief One pass: read the sensor, store the value and tell the others about it */
    void Run();
};

}}
