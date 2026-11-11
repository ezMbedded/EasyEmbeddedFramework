#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "ez_sync.hpp"

namespace ez { namespace application {

using namespace ez::platform;

/* Fixed-time sleep of the calling task. It holds no state, so one object can be shared by all
 * the periodic tasks. */
class AppDelay : public Delay {
public:
    void Sleep(std::uint32_t time_ms) override
    {
        vTaskDelay(pdMS_TO_TICKS(time_ms));
    }
};

}}
