#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ez_sync.hpp"

namespace ez { namespace application {

using namespace ez::platform;

class AppMutex : public Mutex {
public:
    AppMutex(){
        sem = xSemaphoreCreateRecursiveMutex();
    };

    bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) override
    {
        TickType_t  tick; 
        if(sem == nullptr)
        {
            return false;
        }
        
        if(timeout_ms == EZ_SEM_WAIT_FOREVER)
        {
            tick = portMAX_DELAY;
        }
        else
        {
            tick = pdMS_TO_TICKS(timeout_ms);
        }

        return (xSemaphoreTakeRecursive(sem, tick) == pdTRUE);
    }

    bool Unlock() override
    {
        if(sem == nullptr)
        {
            return false;
        }

        return (xSemaphoreGiveRecursive(sem) == pdTRUE);
    }
private:
    SemaphoreHandle_t sem;
};

}}