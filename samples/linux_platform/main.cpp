#include <stdio.h>
#include <cstdlib>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "ez_platform.hpp"

using namespace ez::platform;

/* The platform needs two kinds of synchronization from the application (and a Delay for periodic tasks).
 * This sample implements them with the C++ standard library; on a target
 * they wrap the RTOS (e.g. a FreeRTOS recursive mutex and a task notification). */

/* Recursive mutex: event listeners run with the lock held and may call back into the platform */
class StdMutex : public Mutex {
public:
    bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        if(timeout_ms == EZ_SEM_WAIT_FOREVER)
        {
            mutex.lock();
            return true;
        }
        return mutex.try_lock_for(std::chrono::milliseconds(timeout_ms));
    }

    bool Unlock() override
    {
        mutex.unlock();
        return true;
    }
private:
    std::recursive_timed_mutex mutex;
};

/* Binary signal: Notify() sets it, Wait() blocks until it is set and clears it */
class StdSignal : public Signal {
public:
    bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        std::unique_lock<std::mutex> lock(mutex);
        if(timeout_ms == EZ_SEM_WAIT_FOREVER)
        {
            condition.wait(lock, [this]{ return signaled; });
        }
        else if(condition.wait_for(lock, std::chrono::milliseconds(timeout_ms),
            [this]{ return signaled; }) == false)
        {
            return false;
        }
        signaled = false;
        return true;
    }

    bool Notify() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            signaled = true;
        }
        condition.notify_one();
        return true;
    }

    bool NotifyFromISR() override { return Notify(); }
private:
    std::mutex mutex;
    std::condition_variable condition;
    bool signaled = false;
};

/* Data points of the sample */
enum { DATA_POINT_TEMPERATURE = 0, DATA_POINT_COUNTER = 1 };

static ezDataPoint_t data_points[] = {
    {DATA_POINT_TEMPERATURE, sizeof(float), NULL, false},
    {DATA_POINT_COUNTER, sizeof(uint32_t), NULL, false},
};

/* Memory of the platform, the platform does not allocate */
static uint8_t event_bus_buffer[512];
static uint8_t data_model_buffer[256];

enum { EVENT_TEMPERATURE_CHANGED = 1 };

static int OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    (void)data;
    printf("listener: event %u, %zu bytes of data\n", (unsigned)event_code, data_size);
    return 0;
}

int main()
{
    printf("Hello, EasyEmbeddedFramework!\n");

    StdMutex task_mutex;        /* protects the event bus */
    StdMutex datamodel_mutex;   /* protects the data model, different from the task mutex */
    StdSignal wake_signal;      /* wakes the platform task when an event is sent */

    ezPlatformConfiguration_t config = {
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

    /* One call configures everything. Create the task only after it succeeded */
    if(Platform::instance().Setup(config) != PlatformResult::OK)
    {
        return 1;
    }

    ezEventListener_t listener;
    if(ezEventBus_CreateListener(&listener, OnEvent) != ezSUCCESS
        || Platform::RegisterListener(&listener) != PlatformResult::OK)
    {
        return 1;
    }

    /* On a target this is the entry of the RTOS task */
    std::thread platform_task(Platform::Entry, nullptr);
    platform_task.detach();

    /* Write and read a data point, then tell the others about it */
    float temperature = 21.5f;
    if(Platform::Set(DATA_POINT_TEMPERATURE, temperature) != PlatformResult::OK)
    {
        return 1;
    }

    float read_temperature = 0.0f;
    if(Platform::Get(DATA_POINT_TEMPERATURE, read_temperature) == PlatformResult::OK)
    {
        printf("temperature = %.1f\n", read_temperature);
    }

    uint32_t index = DATA_POINT_TEMPERATURE;
    (void)Platform::SendEvent(EVENT_TEMPERATURE_CHANGED, &index, sizeof(index));

    /* Several data points under one lock, for example to save the dirty ones */
    if(Platform::TakeDataModel(50) == PlatformResult::OK)
    {
        uint32_t dirty = Platform::GetFirstDirtyDataIndex();
        while(dirty != DATA_POINT_INVALID)
        {
            size_t size = 0;
            const void *data = Platform::GetDataPoint(dirty, &size);
            printf("save data point %u (%zu bytes) at %p\n", (unsigned)dirty, size, data);
            Platform::ClearDirtyData(dirty);
            dirty = Platform::GetFirstDirtyDataIndex();
        }
        Platform::GiveDataModel();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));   /* let the task deliver the event */

    /* The platform task never returns and uses the objects above. On a target they live as long as the
     * task. Here the process ends without destroying them */
    fflush(stdout);
    std::_Exit(0);
}
