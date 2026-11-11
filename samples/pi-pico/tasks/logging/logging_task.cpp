#include "logging_task.hpp"
#include "platform/platform_data.hpp"

#define DEBUG_LVL   LVL_DEBUG           /**< logging level */
#define MOD_NAME    "logging"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;
using namespace ez::application;

typedef struct 
{
    uint16_t event_id;
    union
    {
        float f;
        uint32_t u32;
    }event_data;
}LoggingEvent_t;

int LoggingTask::OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    LoggingEvent_t log_event{};

    switch (event_code)
    {
    case EVENT_HEATER_START:
    case EVENT_HEATER_STOP:
    case EVENT_COOLER_START:
    case EVENT_COOLER_STOP:
        log_event.event_id = (uint16_t)event_code;
        log_event.event_data.u32 = 0;
        break;

    case EVENT_DATA_POINT_CHANGED:
    {
        if(data_size != sizeof(uint32_t))
        {
            return 0;
        }
        const uint32_t index = *(const uint32_t*)data;
        if(index != DATA_POINT_TEMPERATURE)
        {
            return 0;
        }

        float temperature = 0.0f;
        if(Platform::Get(index, temperature, 100) != PlatformResult::OK)
        {
            EZERROR("cannot read temperature");
            return 0;
        }
        log_event.event_id = EVENT_DATA_POINT_CHANGED;
        log_event.event_data.f = temperature;
        break;
    }

    default:
        return 0;
    }

    if(instance().Push(&log_event, sizeof(LoggingEvent_t)) == false)
    {
        EZERROR("cannot push event");
        return 0;
    }

    /* the logging task sleeps until it is woken */
    if(Wake() == false)
    {
        EZERROR("cannot wake the logging task");
    }
    return 0;
}

PlatformResult LoggingTask::OnInit()
{
    if(ezRingBuffer_Init(&this->ring_buff, this->buff, sizeof(this->buff)) == false)
    {
        EZERROR("cannot init the ring buffer");
        return PlatformResult::Failed;
    }
    return PlatformResult::OK;
}

bool LoggingTask::Push(const void *data, size_t size)
{
    LockGuard lock;
    if(lock.Locked() == false)
    {
        return false;
    }
    return (ezRingBuffer_Push(&this->ring_buff, (uint8_t*)data, (uint16_t)size) == size);
}

bool LoggingTask::Pop(void *data, size_t size)
{
    LockGuard lock;
    if(lock.Locked() == false)
    {
        return false;
    }
    return (ezRingBuffer_IsEmpty(&this->ring_buff) == false) &&
           (ezRingBuffer_Pop(&this->ring_buff, (uint8_t*)data, (uint16_t)size) == size);
}

void LoggingTask::Run()
{
    LoggingEvent_t event{};
    /* the lock is only held inside Pop(), never while logging */
    while(Pop(&event, sizeof(LoggingEvent_t)) == true)
    {
        switch(event.event_id)
        {
        case EVENT_COOLER_START:
            EZINFO("cooler started");
            break;
        case EVENT_COOLER_STOP:
            EZINFO("cooler stopped");
            break;
        case EVENT_HEATER_START:
            EZINFO("heater started");
            break;
        case EVENT_HEATER_STOP:
            EZINFO("heater stopped");
            break;
        case EVENT_DATA_POINT_CHANGED:
            EZINFO("get new temperature: %f", event.event_data.f);
            break;
        }
    }
}
