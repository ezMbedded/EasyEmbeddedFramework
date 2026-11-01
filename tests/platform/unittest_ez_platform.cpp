/*****************************************************************************
* Filename:         unittest_ez_platform.c
* Author:           Hai Nguyen
* Original Date:    01.11.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_osal.c
 *  @author Hai Nguyen
 *  @date   01.11.2026
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string>
#include <stdexcept>
#include <cstring>
#include "ez_platform.hpp"
#include "ez_data_model.h"
#include "ez_event_bus.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

using namespace ez::platform;

#define DATA_POINT_BOOL_IDX         0U
#define DATA_POINT_UINT8_IDX        1U
#define DATA_POINT_UINT16_IDX       2U
#define DATA_POINT_UINT32_IDX       3U
#define DATA_POINT_INT8_IDX         4U
#define DATA_POINT_INT16_IDX        5U
#define DATA_POINT_INT32_IDX        6U
#define DATA_POINT_FLOAT_IDX        7U
#define DATA_POINT_DOUBLE_IDX       8U
#define DATA_POINT_STRING_IDX       9U
#define DATA_POINT_BLOB_IDX         10U
#define DATA_POINT_STRING_SIZE      64U
#define DATA_POINT_BLOB_SIZE        128U

static ezDataPoint_t data_points[] = {
    {DATA_POINT_BOOL_IDX, sizeof(bool), NULL, false},
    {DATA_POINT_UINT8_IDX, sizeof(uint8_t), NULL, false},
    {DATA_POINT_UINT16_IDX, sizeof(uint16_t), NULL, false},
    {DATA_POINT_UINT32_IDX, sizeof(uint32_t), NULL, false},
    {DATA_POINT_INT8_IDX, sizeof(int8_t), NULL, false},
    {DATA_POINT_INT16_IDX, sizeof(int16_t), NULL, false},
    {DATA_POINT_INT32_IDX, sizeof(int32_t), NULL, false},
    {DATA_POINT_FLOAT_IDX, sizeof(float), NULL, false},
    {DATA_POINT_DOUBLE_IDX, sizeof(double), NULL, false},
    {DATA_POINT_STRING_IDX, DATA_POINT_STRING_SIZE, NULL, false},
    {DATA_POINT_BLOB_IDX, DATA_POINT_BLOB_SIZE, NULL, false},
};


static uint32_t last_event_code;
static uint8_t last_event_data[64];
static size_t last_event_data_size;

static int32_t OnReceiveEvent(uint32_t event_code, const void *event_data, size_t event_data_size)
{
    last_event_code = event_code;
    /* event_data points into the event bus queue, valid only during the callback */
    last_event_data_size = event_data_size;
    if(event_data != nullptr && event_data_size <= sizeof(last_event_data))
    {
        std::memcpy(last_event_data, event_data, event_data_size);
    }
    return 0;
}

class FakeTask : public Task {
public:
    void Stop() override {}
    void Start() override {}

    bool IsInitialized() const { return this->is_initialized; }

    PlatformResult UnregisterListener(void)
    {
        PlatformResult result = Platform::UnregisterListener(&this->event_listener);
        return result;
    }

    bool Init(void)
    {
        if(this->is_configured == false)
        {
            return false;
        }

        if(ezEventBus_CreateListener(&this->event_listener, OnReceiveEvent) == ezSUCCESS)
        {
            if(Platform::RegisterListener(&this->event_listener) == PlatformResult::OK)
            {
                this->event_listener_created = true;
            }
        }

        if(this->event_listener_created == true)
        {
            this->is_initialized = true;
        }
        return this->is_initialized;
    }
protected:
    void Run(void *context) override {}
private:
    bool event_listener_created = false;
};

/* Non recursive mutex, remembers the last timeout */
class FakeMutex : public Mutex {
public:
    bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        last_timeout = timeout_ms;
        if(locked == false)
        {
            locked = true;
            return true;
        }
        return false;
    }

    bool Unlock() override
    {
        locked = false;
        return true;
    }

    bool locked = false;
    uint32_t last_timeout = 0;
};

/* Binary signal: Notify sets, Wait consumes */
class FakeSignal : public Signal {
public:
    bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        last_timeout = timeout_ms;
        wait_calls++;
        if(signaled == true)
        {
            signaled = false;
            return true;
        }
        return false;
    }
    bool Notify() override { signaled = true; notify_calls++; return true; }
    bool NotifyFromISR() override { return Notify(); }

    bool signaled = false;
    int notify_calls = 0;
    int wait_calls = 0;
    uint32_t last_timeout = 0;
};

namespace ez { namespace platform {

struct PlatformTestAccess {
    static void Run(Platform &p, void *context) { p.Run(context); }
    static uint32_t LockTimeout(Platform &p) { return p.config.lock_timeout_ms; }
    static uint32_t TaskDelay(Platform &p) { return p.config.task_delay_ms; }

    /* Return the platform to its power-on state (unconfigured, no bus, no data model) */
    static void Reset(Platform &p)
    {
        p.datamodel_created = false;
        p.is_initialized = false;
        p.is_configured = false;
        p.config = {};
        p.platform_config = {};
        p.is_running = true;

        p.event_bus = {};
        p.event_listener = {};

        p.dataModel = {};
    }
};

}} // namespace ez::platform

class PlatformTestsFixture {
protected:
    static constexpr size_t event_buff_size = 512U;
    uint8_t event_bus_buff[event_buff_size];

    static constexpr size_t model_buff_size = 512U;
    uint8_t data_model_buff[model_buff_size];

    FakeMutex platform_mutex;
    FakeMutex datamodel_mutex;
    FakeMutex task_mutex;
    FakeSignal platform_wake;

    ezPlatformConfiguration_t platform_config = {
        .task_config = {
            .mutex = &platform_mutex,
        },
        .datamodel_mutex = &datamodel_mutex,
        .wake_signal = &platform_wake,
        .event_bus = {
            .buffer = event_bus_buff,
            .size = event_buff_size,
        },
        .data_model = {
            .data_points = data_points,
            .num_of_data_points = sizeof(data_points)/sizeof(ezDataPoint_t),
            .buffer = data_model_buff,
            .buffer_size = model_buff_size,
        },
    };

    ezTaskConfiguration_t task_config = {
        .mutex = &task_mutex,
    };

    Platform &platform = Platform::instance();

    FakeTask fake_task;
public:
    PlatformTestsFixture();
    ~PlatformTestsFixture(){}
protected:
    void InitPlatform()
    {
        PlatformTestAccess::Reset(platform);
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
    }

    /* The platform is not configured: nothing works and nothing runs */
    void CheckPlatformIsNotConfigured()
    {
        uint8_t data[4] = {0};
        uint32_t value = 0U;
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::TaskNotConfigured);
        CHECK(Platform::TakeDataModel() == PlatformResult::TaskNotConfigured);
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::DataModelNotCreated);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK_NOTHROW(Platform::TaskEntry(nullptr));   /* returns at once when not configured */
    }

    void ResetEventData()
    {
        last_event_code = 0;
        memset(last_event_data, 0, sizeof(last_event_data));
        last_event_data_size = 0;
    }
};

TEST_CASE_METHOD(PlatformTestsFixture, "Setup platform", "[platform][Configuration]")
{
    SECTION("Platform is not configured before Setup")
    {
        CheckPlatformIsNotConfigured();
        ezEventListener_t listener = {};
        CHECK(Platform::RegisterListener(&listener) == PlatformResult::TaskNotConfigured);
        CHECK(Platform::UnregisterListener(&listener) == PlatformResult::TaskNotConfigured);
    }

    SECTION("Setup with valid configuration")
    {
        uint8_t data[4] = {1, 2, 3, 4};
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        REQUIRE(fake_task.Init() == true);
        REQUIRE(Platform::SendEvent(0x42, data, sizeof(data)) == PlatformResult::OK);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK(last_event_code == 0x42);

        uint32_t value = 7U;
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::OK);
    }

    SECTION("Setup without a data model")
    {
        platform_config.data_model = {};
        uint8_t data[4] = {1, 2, 3, 4};
        uint32_t value = 7U;
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        CHECK(Platform::SendEvent(0x42, data, sizeof(data)) == PlatformResult::OK);
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::DataModelNotCreated);
        CHECK(Platform::GetFirstDirtyDataIndex() == DATA_POINT_INVALID);
    }

    SECTION("Setup fails when a mutex or the signal is NULL")
    {
        WHEN("Task mutex is NULL")
        {
            platform_config.task_config.mutex = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Data model mutex is NULL")
        {
            platform_config.datamodel_mutex = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Wake signal is NULL")
        {
            platform_config.wake_signal = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Both mutexes are the same object")
        {
            platform_config.datamodel_mutex = platform_config.task_config.mutex;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        CheckPlatformIsNotConfigured();
    }

    SECTION("Setup fails when the event bus memory is invalid")
    {
        WHEN("Buffer is NULL")
        {
            platform_config.event_bus.buffer = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Buffer size is zero")
        {
            platform_config.event_bus.size = 0U;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        CheckPlatformIsNotConfigured();
    }

    SECTION("Setup fails when the data model memory is only partly provided")
    {
        WHEN("Table is NULL")
        {
            platform_config.data_model.data_points = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Number of data points is zero")
        {
            platform_config.data_model.num_of_data_points = 0U;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Buffer is NULL")
        {
            platform_config.data_model.buffer = nullptr;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        WHEN("Buffer size is zero")
        {
            platform_config.data_model.buffer_size = 0U;
            CHECK(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        }

        CheckPlatformIsNotConfigured();
    }

    SECTION("Setup fails when the event bus rejects its memory")
    {
        platform_config.event_bus.size = 1U;
        CHECK(platform.Setup(platform_config) == PlatformResult::EventBusNotCreated);
        CheckPlatformIsNotConfigured();
    }

    SECTION("Setup fails when the data model rejects its memory")
    {
        platform_config.data_model.buffer_size = 1U;
        CHECK(platform.Setup(platform_config) == PlatformResult::DataModelNotCreated);
        CheckPlatformIsNotConfigured();
    }

    SECTION("A valid configuration replaced by an invalid one stops the platform")
    {
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        platform_config.wake_signal = nullptr;
        REQUIRE(platform.Setup(platform_config) == PlatformResult::InvalidParams);
        CheckPlatformIsNotConfigured();
    }

    SECTION("Setup again re-creates the event bus, the listeners are lost")
    {
        uint8_t data[4] = {1, 2, 3, 4};
        InitPlatform();
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        REQUIRE(fake_task.Init() == true);

        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        REQUIRE(Platform::SendEvent(0x42, data, sizeof(data)) == PlatformResult::OK);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK(last_event_code == 0);
    }

    SECTION("Defaults are resolved once in Setup")
    {
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        CHECK(PlatformTestAccess::LockTimeout(platform) == EZ_DEFAULT_LOCK_TIMEOUT_MS);
        CHECK(PlatformTestAccess::TaskDelay(platform) == EZ_DEFAULT_TASK_DELAY_MS);

        platform_config.task_config.lock_timeout_ms = 123U;
        platform_config.task_config.task_delay_ms = 456U;
        REQUIRE(platform.Setup(platform_config) == PlatformResult::OK);
        CHECK(PlatformTestAccess::LockTimeout(platform) == 123U);
        CHECK(PlatformTestAccess::TaskDelay(platform) == 456U);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Initialize Task",
    "[platform][task]")
{
    SECTION("Initialize task with invalid configuration")
    {
        InitPlatform();

        WHEN("Task is not configured")
        {
            THEN("Initialization fails")
            {
                fake_task.Init();
                CHECK(fake_task.IsInitialized() == false);
            }
        }

        WHEN("Task is configured with invalid configuration")
        {
            ezTaskConfiguration_t task_config_invalid1 = {
                .mutex = nullptr,
            };

            CHECK(fake_task.Setup(task_config_invalid1) == PlatformResult::InvalidParams);
            THEN("Initialization fails")
            {
                fake_task.Init();
                CHECK(fake_task.IsInitialized() == false);
            }
        }
    }

    SECTION("Initialize task when platform is not set up")
    {
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        fake_task.Init();
        CHECK(fake_task.IsInitialized() == false);
    }

    SECTION("Initialize task with valid configuration")
    {
        InitPlatform();
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        fake_task.Init();
        CHECK(fake_task.IsInitialized() == true);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Task send event to platform",
    "[platform][task]")
{
    InitPlatform();
    REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
    fake_task.Init();
    REQUIRE(fake_task.IsInitialized() == true);

    SECTION("Task sends event to platform")
    {
        uint32_t event_code = 0x12345678;
        uint8_t event_data[4] = {0x01, 0x02, 0x03, 0x04};
        size_t event_data_size = sizeof(event_data);

        REQUIRE(platform.SendEvent(event_code, event_data, event_data_size) == PlatformResult::OK);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK(last_event_code == event_code);
        CHECK(last_event_data_size == event_data_size);
        CHECK(std::memcmp(last_event_data, event_data, event_data_size) == 0);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Task unregister listener from platform",
    "[platform][task]")
{
    InitPlatform();
    REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
    fake_task.Init();
    REQUIRE(fake_task.IsInitialized() == true);

    SECTION("Task unregisters listener from platform")
    {
        CHECK(fake_task.UnregisterListener() == PlatformResult::OK);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Task get data from platform",
    "[platform][task]")
{
    PlatformResult result = PlatformResult::Failed;
    InitPlatform();
    REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
    fake_task.Init();
    REQUIRE(fake_task.IsInitialized() == true);

    SECTION("Task get/set data point")
    {
        uint32_t index = DATA_POINT_UINT32_IDX;
        uint32_t value = 0xDEADBEEF;
        result = platform.SetDataPoint(index, &value, sizeof(value));
        REQUIRE(result == PlatformResult::OK);

        size_t data_size = 0;
        const void *data = platform.GetDataPoint(index, &data_size);
        REQUIRE(data != nullptr);
        REQUIRE(data_size == sizeof(value));
        CHECK(std::memcmp(data, &value, sizeof(value)) == 0);
    }

    SECTION("Task get/set data point with event bus")
    {
        uint32_t index = DATA_POINT_UINT32_IDX;
        uint32_t value = 0xDEADBEEF;
        result = platform.SetDataPoint(index, &value, sizeof(value));
        REQUIRE(result == PlatformResult::OK);

        uint32_t event_code = 0x12345678;
        uint32_t event_data = DATA_POINT_UINT32_IDX;
        size_t event_data_size = sizeof(event_data);

        REQUIRE(platform.SendEvent(event_code, (void*)&event_data, event_data_size) == PlatformResult::OK);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK(last_event_code == event_code);
        CHECK(last_event_data_size == event_data_size);
        CHECK(std::memcmp(last_event_data, &event_data, event_data_size) == 0);

        size_t data_size = 0;
        const void *data = platform.GetDataPoint(event_data, &data_size);
        REQUIRE(data != nullptr);
        REQUIRE(data_size == sizeof(value));
        CHECK(std::memcmp(data, &value, sizeof(value)) == 0);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Task data point dirty flag",
    "[platform][task]")
{
    PlatformResult result = PlatformResult::Failed;
    InitPlatform();
    REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
    fake_task.Init();
    REQUIRE(fake_task.IsInitialized() == true);

    uint32_t index = DATA_POINT_UINT32_IDX;
    uint32_t value = 0xDEADBEEF;
    result = platform.SetDataPoint(index, &value, sizeof(value));
    REQUIRE(result == PlatformResult::OK);

    SECTION("Get first dirty data index")
    {
        CHECK(platform.GetFirstDirtyDataIndex() == index);
    }

    SECTION("Clear dirty data index")
    {
        REQUIRE(platform.ClearDirtyData(index) == PlatformResult::OK);
        CHECK(platform.GetFirstDirtyDataIndex() == DATA_POINT_INVALID);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform start/stop",
    "[platform][task]")
{
    InitPlatform();
    ResetEventData();
    REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
    REQUIRE(fake_task.Init() == true);
    REQUIRE(fake_task.IsInitialized() == true);

    SECTION("Platform is stop")
    {
        uint32_t event_code = 0x12345678;
        uint8_t event_data[4] = {0x01, 0x02, 0x03, 0x04};
        size_t event_data_size = sizeof(event_data);
        REQUIRE(platform.SendEvent(event_code, event_data, event_data_size) == PlatformResult::OK);

        GIVEN("Platform is stopped")
        {
            platform.Stop();
            PlatformTestAccess::Run(platform, nullptr);
            THEN("No event is notified on the event bus")
            {
                CHECK(last_event_code == 0);
                CHECK(last_event_data_size == 0);
            }
        }

        GIVEN("Platform is started")
        {
            platform.Start();
            PlatformTestAccess::Run(platform, nullptr);
            THEN("Task receives event from platform")
            {
                CHECK(last_event_code == event_code);
                CHECK(last_event_data_size == event_data_size);
                CHECK(std::memcmp(last_event_data, event_data, event_data_size) == 0);
            }
        }
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform event bus error handling",
    "[platform][event_bus]")
{
    InitPlatform();

    SECTION("Register and unregister invalid listener")
    {
        CHECK(Platform::RegisterListener(nullptr) == PlatformResult::Failed);
        CHECK(Platform::UnregisterListener(nullptr) == PlatformResult::Failed);
    }

    SECTION("Register and unregister time out when the mutex is not available")
    {
        ezEventListener_t listener = {};
        REQUIRE(ezEventBus_CreateListener(&listener, OnReceiveEvent) == ezSUCCESS);
        REQUIRE(platform_mutex.Lock() == true);
        CHECK(Platform::RegisterListener(&listener) == PlatformResult::Timeout);
        CHECK(Platform::UnregisterListener(&listener) == PlatformResult::Timeout);
        platform_mutex.Unlock();
        CHECK(Platform::RegisterListener(&listener) == PlatformResult::OK);
    }

    SECTION("Send event times out when the mutex is not available")
    {
        uint8_t data[4] = {0};
        REQUIRE(platform_mutex.Lock() == true);
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::Timeout);
        platform_mutex.Unlock();
    }

    SECTION("Send event with NULL data and non-zero size fails")
    {
        CHECK(Platform::SendEvent(0x01, nullptr, 4U) == PlatformResult::InvalidParams);
    }

    SECTION("Send event without data is valid")
    {
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        REQUIRE(fake_task.Init() == true);
        CHECK(Platform::SendEvent(0x77, nullptr, 0U) == PlatformResult::OK);
        PlatformTestAccess::Run(platform, nullptr);
        CHECK(last_event_code == 0x77);
        CHECK(last_event_data_size == 0U);
    }

    SECTION("Send event fails when event is bigger than the event bus buffer")
    {
        uint8_t big_data[event_buff_size * 2] = {0};
        CHECK(Platform::SendEvent(0x01, big_data, sizeof(big_data)) == PlatformResult::Failed);
    }

    SECTION("The configured lock timeout is used")
    {
        uint8_t data[4] = {0};
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::OK);
        CHECK(platform_mutex.last_timeout == EZ_DEFAULT_LOCK_TIMEOUT_MS);

        platform_config.task_config.lock_timeout_ms = 123U;
        InitPlatform();
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::OK);
        CHECK(platform_mutex.last_timeout == 123U);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform data model error handling",
    "[platform][data_model]")
{
    SECTION("Data model API fails when data model is not created")
    {
        uint32_t value = 0U;
        size_t size = 0U;
        CHECK(Platform::SetDataPoint(DATA_POINT_UINT32_IDX, &value, sizeof(value)) == PlatformResult::DataModelNotCreated);
        CHECK(Platform::GetDataPoint(DATA_POINT_UINT32_IDX, &size) == nullptr);
        CHECK(Platform::GetFirstDirtyDataIndex() == DATA_POINT_INVALID);
        CHECK(Platform::ClearDirtyData(DATA_POINT_UINT32_IDX) == PlatformResult::DataModelNotCreated);
    }

    SECTION("Set data point with wrong size or unknown index fails")
    {
        InitPlatform();
        uint32_t value = 0U;
        CHECK(Platform::SetDataPoint(DATA_POINT_UINT32_IDX, &value, sizeof(uint8_t)) == PlatformResult::Failed);
        CHECK(Platform::SetDataPoint(0xFFFFU, &value, sizeof(value)) == PlatformResult::Failed);
        CHECK(Platform::SetDataPoint(DATA_POINT_UINT32_IDX, nullptr, sizeof(value)) == PlatformResult::InvalidParams);
    }

    SECTION("Get data point with unknown index fails")
    {
        InitPlatform();
        size_t size = 0;
        CHECK(Platform::GetDataPoint(0xFFFFU, &size) == nullptr);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform typed data point access",
    "[platform][data_model]")
{
    SECTION("Get and Set fail when the data model is not created")
    {
        uint32_t value = 0U;
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::DataModelNotCreated);
        CHECK(Platform::Get(DATA_POINT_UINT32_IDX, value) == PlatformResult::DataModelNotCreated);
    }

    InitPlatform();

    SECTION("Set then Get returns the value")
    {
        uint32_t value = 0xDEADBEEF;
        uint32_t read_value = 0U;
        REQUIRE(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::OK);
        REQUIRE(Platform::Get(DATA_POINT_UINT32_IDX, read_value) == PlatformResult::OK);
        CHECK(read_value == value);
        CHECK(Platform::GetFirstDirtyDataIndex() == DATA_POINT_UINT32_IDX);
    }

    SECTION("Float and array values work")
    {
        float f = 21.5f;
        float read_f = 0.0f;
        REQUIRE(Platform::Set(DATA_POINT_FLOAT_IDX, f) == PlatformResult::OK);
        REQUIRE(Platform::Get(DATA_POINT_FLOAT_IDX, read_f) == PlatformResult::OK);
        CHECK(read_f == Catch::Approx(21.5f));

        char text[DATA_POINT_STRING_SIZE] = "hello";
        char read_text[DATA_POINT_STRING_SIZE] = {0};
        REQUIRE(Platform::Set(DATA_POINT_STRING_IDX, text) == PlatformResult::OK);
        REQUIRE(Platform::Get(DATA_POINT_STRING_IDX, read_text) == PlatformResult::OK);
        CHECK(std::strcmp(read_text, "hello") == 0);
    }

    SECTION("Get with a type of the wrong size fails")
    {
        uint8_t small = 0U;
        CHECK(Platform::Get(DATA_POINT_UINT32_IDX, small) == PlatformResult::InvalidParams);
    }

    SECTION("Unknown index fails")
    {
        uint32_t value = 0U;
        CHECK(Platform::Get(0xFFFFU, value) == PlatformResult::Failed);
        CHECK(Platform::Set(0xFFFFU, value) == PlatformResult::Failed);
    }

    SECTION("Failures release the data model mutex")
    {
        uint8_t small = 0U;
        uint32_t value = 0U;
        CHECK(Platform::Get(DATA_POINT_UINT32_IDX, small) == PlatformResult::InvalidParams);
        CHECK(Platform::Get(0xFFFFU, value) == PlatformResult::Failed);
        CHECK(Platform::Set(0xFFFFU, value) == PlatformResult::Failed);
        CHECK(datamodel_mutex.Lock() == true);
        datamodel_mutex.Unlock();
    }

    SECTION("Get and Set time out when the data model is locked")
    {
        uint32_t value = 0U;
        REQUIRE(Platform::TakeDataModel() == PlatformResult::OK);
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::Timeout);
        CHECK(Platform::Get(DATA_POINT_UINT32_IDX, value) == PlatformResult::Timeout);
        CHECK(Platform::GiveDataModel() == PlatformResult::OK);
        CHECK(Platform::Set(DATA_POINT_UINT32_IDX, value) == PlatformResult::OK);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform data model mutex",
    "[platform][data_model]")
{
    uint32_t value = 0xDEADBEEF;

    InitPlatform();

    SECTION("Lock and unlock use the data model mutex, not the task mutex")
    {
        REQUIRE(Platform::TakeDataModel(77U) == PlatformResult::OK);
        CHECK(datamodel_mutex.last_timeout == 77U);
        CHECK(datamodel_mutex.Lock() == false);
        CHECK(Platform::TakeDataModel() == PlatformResult::Timeout);
        CHECK(platform_mutex.Lock() == true);
        platform_mutex.Unlock();

        /* event bus works while the data model is locked */
        uint8_t data[4] = {0};
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::OK);

        CHECK(Platform::GiveDataModel() == PlatformResult::OK);
        CHECK(datamodel_mutex.Lock() == true);
        datamodel_mutex.Unlock();
    }

    SECTION("Data model functions do not lock by themselves")
    {
        REQUIRE(Platform::TakeDataModel() == PlatformResult::OK);
        CHECK(Platform::SetDataPoint(DATA_POINT_UINT32_IDX, &value, sizeof(value)) == PlatformResult::OK);
        size_t size = 0;
        CHECK(Platform::GetDataPoint(DATA_POINT_UINT32_IDX, &size) != nullptr);
        CHECK(Platform::GiveDataModel() == PlatformResult::OK);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform wake signal",
    "[platform][event_bus]")
{
    uint8_t data[4] = {0};
    InitPlatform();

    SECTION("SendEvent wakes the platform task")
    {
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::OK);
        CHECK(platform_wake.notify_calls == 1);
        CHECK(platform_wake.signaled == true);
    }

    SECTION("Failed SendEvent does not wake the platform task")
    {
        uint8_t big_data[event_buff_size * 2] = {0};
        CHECK(Platform::SendEvent(0x01, big_data, sizeof(big_data)) == PlatformResult::Failed);
        CHECK(Platform::SendEvent(0x01, nullptr, 4U) == PlatformResult::InvalidParams);
        CHECK(platform_wake.notify_calls == 0);
    }

    SECTION("SendEvent does not wake when the lock times out")
    {
        REQUIRE(platform_mutex.Lock() == true);
        CHECK(Platform::SendEvent(0x01, data, sizeof(data)) == PlatformResult::Timeout);
        platform_mutex.Unlock();
        CHECK(platform_wake.notify_calls == 0);
    }
}


TEST_CASE_METHOD(
    PlatformTestsFixture,
    "Platform task entry",
    "[platform][task]")
{
    /* TaskEntry never returns, the signal throws to leave the loop */
    struct ThrowingSignal : FakeSignal {
        bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
        {
            FakeSignal::Wait(timeout_ms);
            throw std::runtime_error("leave TaskEntry loop");
        }
    } wake;

    platform_config.wake_signal = &wake;

    SECTION("Task entry waits on the wake signal with the configured timeout")
    {
        platform_config.task_config.task_delay_ms = 25U;
        InitPlatform();
        CHECK_THROWS_AS(Platform::TaskEntry(nullptr), std::runtime_error);
        CHECK(wake.wait_calls == 1);
        CHECK(wake.last_timeout == 25U);
    }

    SECTION("Task entry uses the default timeout when none is configured")
    {
        InitPlatform();
        CHECK_THROWS_AS(Platform::TaskEntry(nullptr), std::runtime_error);
        CHECK(wake.last_timeout == EZ_DEFAULT_TASK_DELAY_MS);
    }

    SECTION("Task entry handles the events before it sleeps")
    {
        uint8_t data[4] = {1, 2, 3, 4};
        InitPlatform();
        REQUIRE(fake_task.Setup(task_config) == PlatformResult::OK);
        REQUIRE(fake_task.Init() == true);
        REQUIRE(Platform::SendEvent(0x42, data, sizeof(data)) == PlatformResult::OK);
        CHECK_THROWS_AS(Platform::TaskEntry(nullptr), std::runtime_error);
        CHECK(last_event_code == 0x42);
    }

    SECTION("Task entry does not run when the platform is not configured")
    {
        CHECK_NOTHROW(Platform::TaskEntry(nullptr));
        CHECK(wake.wait_calls == 0);
    }
}


/******************************************************************************
* Internal functions
*******************************************************************************/
PlatformTestsFixture::PlatformTestsFixture()
{
    PlatformTestAccess::Reset(platform);
    ResetEventData();
}
