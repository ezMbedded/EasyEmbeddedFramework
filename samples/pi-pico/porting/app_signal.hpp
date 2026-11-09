#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ez_sync.hpp"

namespace ez { namespace application {

using namespace ez::platform;

class AppSignal : public Signal {
public:
    AppSignal()
    {
        // Static allocation: no heap (framework rule 1).
        // Needs configSUPPORT_STATIC_ALLOCATION = 1 in FreeRTOSConfig.h.
        sem = xSemaphoreCreateBinary();
    }

    ~AppSignal() override
    {
        if (sem != nullptr)
        {
            vSemaphoreDelete(sem);
        }
    }

    bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        if (sem == nullptr)
        {
            return false;
        }

        TickType_t tick = (timeout_ms == EZ_SEM_WAIT_FOREVER)
                              ? portMAX_DELAY
                              : pdMS_TO_TICKS(timeout_ms);

        return (xSemaphoreTake(sem, tick) == pdTRUE);   // taking clears it
    }

    bool Notify() override
    {
        if (sem == nullptr)
        {
            return false;
        }

        // pdFALSE only means the signal is already set, which is fine:
        // several Notify() calls before a Wait() count as one.
        BaseType_t result = xSemaphoreGive(sem);
        (void)result;
        return true;
    }

    bool NotifyFromISR() override
    {
        if (sem == nullptr)
        {
            return false;
        }

        BaseType_t woken = pdFALSE;
        BaseType_t result = xSemaphoreGiveFromISR(sem, &woken);
        (void)result;
        portYIELD_FROM_ISR(woken);  // context switch if a higher-priority task woke
        return true;
    }

private:
    SemaphoreHandle_t sem = nullptr;
};

}}
