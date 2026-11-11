#include "platform_config.hpp"
#include "platform_data.hpp"
#include "ez_platform.hpp"
#include "app_mutex.hpp"
#include "app_signal.hpp"
#include "app_delay.hpp"
#include "sensor/sensor_task.hpp"
#include "climate_control/climate_control_task.hpp"
#include "logging/logging_task.hpp"

#define DEBUG_LVL   LVL_DEBUG   /**< logging level */
#define MOD_NAME    "config"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;
using namespace ez::application;

static ezDataPoint_t data_points[] = {
    {DATA_POINT_TEMPERATURE, sizeof(float), NULL, false},
    {DATA_POINT_COUNTER, sizeof(uint32_t), NULL, false},
};

static uint8_t event_bus_buffer[512];
static uint8_t data_model_buffer[256];

/* Platform */
static AppMutex task_mutex;        /* protects the event bus */
static AppMutex datamodel_mutex;   /* protects the data model, different from the task mutex */
static AppSignal wake_signal;      /* wakes the platform task when an event is sent */
static ezEventListener_t listener;

/* The sensor task is periodic: it sleeps with a delay, needs no mutex and no listener */
static AppDelay sensor_delay;
static const ezTaskConfiguration_t sensor_config = {
    .task_delay_ms = 3000,
    .delay = &sensor_delay,
};

/* The climate control task is woken by its own listener */
static AppSignal climate_control_signal;
static const ezTaskConfiguration_t climate_control_config = {
    .wake_signal = &climate_control_signal,
};

/* The logging task is woken by its own listener and shares its ring buffer with the platform task */
static AppMutex logging_mutex;
static AppSignal logging_signal;
static const ezTaskConfiguration_t logging_config = {
    .mutex = &logging_mutex,
    .lock_timeout_ms = 100,
    .task_delay_ms = 5000,      /* only a safety timeout, the listener wakes the task at once */
    .wake_signal = &logging_signal,
};

static const ezPlatformConfiguration_t config = {
    .task_config = {
        .mutex = &task_mutex,
        .wake_signal = &wake_signal,
    },
    .datamodel_mutex = &datamodel_mutex,
    .event_bus = {
        .buffer = event_bus_buffer,
        .size = sizeof(event_bus_buffer),
    },
    .data_model = {
        .data_points = data_points,
        .num_of_data_points = sizeof(data_points)/sizeof(data_points[0]),
        .buffer = data_model_buffer,
        .buffer_size = sizeof(data_model_buffer),
    },
};

static int OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    (void)data;
    switch (event_code)
    {
    case EVENT_DATA_POINT_CHANGED:
    {
        if(data_size != sizeof(uint32_t))
        {
            break;
        }
        const uint32_t index = *(uint32_t*)data;
        switch (index)
        {
            case DATA_POINT_COUNTER:
            {
                EZINFO("System alive!");
                break;
            }
            
            default:
                break;
        }
        break;
    }
        
    
    default:
        break;
    }
    return 0;
}

static void alive_task(void *arg)
{
    (void)arg;
    uint32_t counter = 0;
    while(1)
    {
        counter++;
        uint32_t index = DATA_POINT_COUNTER;
        if(Platform::Set(index, counter, 100) != PlatformResult::OK)
        {
            EZERROR("cannot store the counter");
        }
        else if(Platform::SendEvent(EVENT_DATA_POINT_CHANGED, &index, sizeof(index)) != PlatformResult::OK)
        {
            EZERROR("cannot send the counter event");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

bool appPlatformConfig_Init(void)
{
    /* One call per task: configure it, then create the RTOS task only after it succeeded */
    if(Platform::instance().Setup(config) != PlatformResult::OK)
    {
        return false;
    }

    if(ezEventBus_CreateListener(&listener, OnEvent) != ezSUCCESS
        || Platform::RegisterListener(&listener) != PlatformResult::OK)
    {
        return false;
    }

    if(SensorTask::Init(sensor_config) != PlatformResult::OK
        || ClimateControlTask::Init(climate_control_config, ClimateControlTask::OnEvent) != PlatformResult::OK
        || LoggingTask::Init(logging_config, LoggingTask::OnEvent) != PlatformResult::OK)
    {
        return false;
    }

    if(xTaskCreate(Platform::Entry, "platform", 2048, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
        || xTaskCreate(SensorTask::Entry, "sensor", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
        || xTaskCreate(ClimateControlTask::Entry, "climate", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
        || xTaskCreate(LoggingTask::Entry, "logging", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS
        || xTaskCreate(alive_task, "alive", 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        return false;
    }
    return true;
}
