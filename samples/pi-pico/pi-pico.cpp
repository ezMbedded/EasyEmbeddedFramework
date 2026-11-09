#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "ez_platform.hpp"
#include "platform/platform_config.hpp"
#include "platform/platform_data.hpp"


#define DEBUG_LVL   LVL_DEBUG   /**< logging level */
#define MOD_NAME    "main"       /**< module name */
#include "ez_logging.h"

using namespace ez::platform;


void vApplicationMallocFailedHook(void) {
    EZINFO("FreeRTOS: malloc failed\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    EZINFO("FreeRTOS: stack overflow in task %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main() {
    stdio_init_all();

    EZINFO("EasyEmbeddedFramework sample for Raspberry Pi Pico");
    if(appPlatformConfig_Init() == false)
    {
        return -1;
    }

    
    vTaskStartScheduler();
    for (;;){
    }
}
