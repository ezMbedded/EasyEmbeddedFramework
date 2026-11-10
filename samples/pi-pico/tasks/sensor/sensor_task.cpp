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

static int OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    return 0;
}

static SensorTask *self = nullptr;


PlatformResult SensorTask::Init(void)
{
    if(this->is_configured == false)
    {
        EZERROR("task is not configured.");
        return PlatformResult::TaskNotConfigured;
    }

    if(ezEventBus_CreateListener(&this->event_listener, OnEvent) != ezSUCCESS)
    {
        EZERROR("cannot create event bus");
        return PlatformResult::Failed;
    }

    if(Platform::instance().RegisterListener(&this->event_listener) != PlatformResult::OK)
    {
        EZERROR("cannot register listerner");
        return PlatformResult::Failed;
    }

    this->is_initialized = true;
    this->running = true;
    self = this;
    return PlatformResult::OK;
}

void SensorTask::Stop(void)
{
    if (this->running)
    {
        this->running = false;
    }
    
}

void SensorTask::Start(void)
{
    if(!this->running)
    {
        this->running = true;
    }
}

void ez::application::SensorTask::TaskEntry(void *context)
{
    if(self->is_configured == false)
    {
        EZERROR("task not configured. Call Setup()");
        return;
    }

    if(self->is_initialized == false)
    {
        EZERROR("task not init. Call Init()");
        return;
    }
    
    self->Run(context);
}

void SensorTask::Run(void *context)
{
    (void)context;
    float temperature = randomFloat(-20.0f, 40.0f);

    const uint32_t index = DATA_POINT_TEMPERATURE;

    /* Set() takes the data model mutex; the listener reads it from the platform task */
    if(Platform::Set(index, temperature, 100) != PlatformResult::OK)
    {
        return;
    }

    if(Platform::instance().SendEvent(
        EVENT_DATA_POINT_CHANGED,
        &index,
        sizeof(index)) != PlatformResult::OK)
    {
        return;
    }
}
