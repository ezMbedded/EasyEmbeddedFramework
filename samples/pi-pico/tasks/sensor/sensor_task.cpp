#include "sensor_task.hpp"
#include "platform/platform_data.hpp"
#include "pico/rand.h"

#define DEBUG_LVL   LVL_DEBUG           /**< logging level */
#define MOD_NAME    "sensor"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;
using namespace ez::application;

static float randomFloat(float min, float max) {
    /* 24 random bits -> [0, 1), then scale into [min, max) */
    const float unit = (float)(get_rand_32() >> 8) / 16777216.0f;
    return min + (max - min) * unit;
}

void SensorTask::Run()
{
    float temperature = randomFloat(-20.0f, 40.0f);

    const uint32_t index = DATA_POINT_TEMPERATURE;

    /* Set() takes the data model mutex; the listener reads it from the platform task */
    if(Platform::Set(index, temperature, 100) != PlatformResult::OK)
    {
        EZERROR("cannot store the temperature");
        return;
    }

    if(Platform::SendEvent(
        EVENT_DATA_POINT_CHANGED,
        &index,
        sizeof(index)) != PlatformResult::OK)
    {
        EZERROR("cannot send the temperature event");
    }
}
