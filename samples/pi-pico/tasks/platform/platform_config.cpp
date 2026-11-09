#include "platform_config.hpp"
#include "platform_data.hpp"
#include "ez_platform.hpp"
#include "app_mutex.hpp"
#include "app_signal.hpp"
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

static AppMutex task_mutex;        /* protects the event bus */
static AppMutex datamodel_mutex;   /* protects the data model, different from the task mutex */
static AppSignal wake_signal;      /* wakes the platform task when an event is sent */
static ezEventListener_t listener;

static SensorTask sensor;
static AppMutex sensor_mutex;
static ezTaskConfiguration_t sensor_config = {
    .mutex = &sensor_mutex,
    .lock_timeout_ms = 100,
    .task_delay_ms = 1000,
};

static ClimateControlTask climate_control(20.0f);
static AppMutex climate_control_mutex;
static ezTaskConfiguration_t climate_control_config = {
    .mutex = &climate_control_mutex,
    .lock_timeout_ms = 100,
    .task_delay_ms = 1000,
};


static LoggingTask logging;
static AppMutex logging_mutex;
static ezTaskConfiguration_t logging_config = {
    .mutex = &logging_mutex,
    .lock_timeout_ms = 100,
    .task_delay_ms = 1000,
};

static const ezPlatformConfiguration_t config = {
    .task_config = {
        .mutex = &task_mutex,
    },
    .datamodel_mutex = &datamodel_mutex,
    .wake_signal = &wake_signal,
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

static void main_task(void *arg) {
    (void)arg;
    EZINFO("Hello, EasyEmbeddedFramework!");
    Platform::TaskEntry(nullptr);
    EZINFO("Platform task exited");
}

static void alive_task(void *arg)
{
    uint32_t counter = 0;
    while(1)
    {
        counter++;
        uint32_t index = DATA_POINT_COUNTER;
        (void)Platform::Set(index, &counter, 100);
        (void)Platform::SendEvent(EVENT_DATA_POINT_CHANGED, &index, sizeof(index));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void sensor_task(void *arg)
{
    (void)arg;
    
    while(1)
    {
        sensor.TaskEntry(nullptr);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void climate_control_task(void *arg)
{
    (void)arg;
    
    while(1)
    {
        climate_control.TaskEntry(nullptr);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void logging_task(void *arg)
{
    (void)arg;
    
    while(1)
    {
        logging.TaskEntry(nullptr);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool appPlatformConfig_Init(void)
{

    /* One call configures everything. Create the task only after it succeeded */
    if(Platform::instance().Setup(config) != PlatformResult::OK)
    {
        return false;
    }

    if(ezEventBus_CreateListener(&listener, OnEvent) != ezSUCCESS
        || Platform::RegisterListener(&listener) != PlatformResult::OK)
    {
        return false;
    }

    sensor.Setup(sensor_config);
    sensor.Init();

    climate_control.Setup(climate_control_config);
    climate_control.Init();

    logging.Setup(logging_config);
    logging.Init();

    xTaskCreate(main_task, "main", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(sensor_task, "sensor", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(alive_task, "alive", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(climate_control_task, "climate", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(logging_task, "logging", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);

    return true;
}
