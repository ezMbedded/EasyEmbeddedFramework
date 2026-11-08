#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

static void main_task(void *arg) {
    (void)arg;
    uint32_t count = 0;
    for (;;) {
        count++;
        printf("Hello, world! (%lu)\n", (unsigned long)count);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vApplicationMallocFailedHook(void) {
    printf("FreeRTOS: malloc failed\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    printf("FreeRTOS: stack overflow in task %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main() {
    stdio_init_all();
    xTaskCreate(main_task, "main", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
    for (;;){
    }
}
