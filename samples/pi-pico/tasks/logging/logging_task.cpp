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

static LoggingTask *self = nullptr;

static void PushEvent(const LoggingEvent_t &log_event)
{
    if(self->Push(&log_event, sizeof(LoggingEvent_t)) == false)
    {
        EZERROR("cannot push event");
    }
}

static int OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    LoggingEvent_t log_event{};

    switch (event_code)
    {
    case EVENT_HEATER_START:
        log_event.event_id = EVENT_HEATER_START;
        log_event.event_data.u32 = 0;
        PushEvent(log_event);
        break;
    
    case EVENT_HEATER_STOP:
        log_event.event_id = EVENT_HEATER_STOP;
        log_event.event_data.u32 = 0;
        PushEvent(log_event);
        break;

    case EVENT_COOLER_START:
        log_event.event_id = EVENT_COOLER_START;
        log_event.event_data.u32 = 0;
        PushEvent(log_event);
        break;

    case EVENT_COOLER_STOP:
        log_event.event_id = EVENT_COOLER_STOP;
        log_event.event_data.u32 = 0;
        PushEvent(log_event);
        break;

    case EVENT_DATA_POINT_CHANGED:
    {
        if(data_size != sizeof(uint32_t))
        {
            break;
        }
        const uint32_t index = *(uint32_t*)data;
        switch (index)
        {
            case DATA_POINT_TEMPERATURE:
            {
                float temperature = 0.0f;

                if(Platform::Get(index, temperature, 100) != PlatformResult::OK)
                {
                    EZERROR("cannot read temperature");
                    break;
                }
                log_event.event_id = EVENT_DATA_POINT_CHANGED;
                log_event.event_data.f = temperature;
                PushEvent(log_event);
                break;
            }
        }
    }

    default:
        break;
    }
    return 0;
}


PlatformResult LoggingTask::Init(void)
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

    if(ezRingBuffer_Init(&this->ring_buff, this->buff, sizeof(this->buff)) == false)
    {
        EZERROR("init ring buffer failed");
        return PlatformResult::Failed;
    }

    this->is_initialized = true;
    this->running = true;
    self = this;
    return PlatformResult::OK;
}

bool LoggingTask::Push(const void *data, size_t size)
{
    bool ok = false;
    if(this->config.mutex->Lock(this->config.lock_timeout_ms) == true)
    {
        ok = (ezRingBuffer_Push(&this->ring_buff, (uint8_t*)data, (uint16_t)size) == size);
        this->config.mutex->Unlock();
    }
    return ok;
}

bool LoggingTask::Pop(void *data, size_t size)
{
    bool ok = false;
    if(this->config.mutex->Lock(this->config.lock_timeout_ms) == true)
    {
        ok = (ezRingBuffer_IsEmpty(&this->ring_buff) == false) &&
             (ezRingBuffer_Pop(&this->ring_buff, (uint8_t*)data, (uint16_t)size) == size);
        this->config.mutex->Unlock();
    }
    return ok;
}

void LoggingTask::Stop(void)
{
    if (this->running)
    {
        this->running = false;
    }
    
}

void LoggingTask::Start(void)
{
    if(!this->running)
    {
        this->running = true;
    }
}

void LoggingTask::TaskEntry(void *context)
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

void LoggingTask::Run(void *context)
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
