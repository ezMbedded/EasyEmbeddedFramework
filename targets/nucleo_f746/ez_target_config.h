#ifndef _EZ_TARGET_CONFIG_H
#define _EZ_TARGET_CONFIG_H
#define EZ_SERVICE 1 /* Enable HAL layer */
#define EZ_MIDDLEWARES 1 /* Enable Middlewares layer */
#define EZ_HAL 1 /* Enable HAL layer */

/* Configure Utilities modules */
#define EZ_LOGGING 1 /* Enable logging feature */
#define EZ_LOGGING_COLOR 0 /* Enable logging using color feature */
#define EZ_LINKEDLIST 1 /* Enable linked list feature */
#define EZ_HEXDUMP 1 /* Enable hexdump feature */
#define EZ_RING_BUFFER 1 /* Enable ring buffer feature */
#define EZ_ASSERT 0 /* Enable assert feature */
#define EZ_STATIC_ALLOC 1 /* Enable static allocation feature */
#define EZ_SYS_ERROR 1 /* Enable system error feature */
#define EZ_QUEUE 1 /* Enable queue feature */

/* Configure Service modules */
#define EZ_EVENT_NOTIFIER 0 /* Enable the Event Notifier module */
#define EZ_KERNEL 0 /* Enable the Kernel service */
#define EZ_TASK_WORKER 0 /* Enable the task worker */
#define EZ_STATE_MACHINE 0 /* Enable state machine */

/* Configure application framework */
#define DATA_MODEL 0 /* Enable the Event Notifier module */
#define EZ_CLI 0 /* Enable command line interface */
#define EZ_RPC 0 /* Enable remote procedure call */
#define EZ_IPC 0 /* Enable inter process communication */
#define EZ_HAL_DRIVER 0 /* Enable the Driver module */
#define EZ_UART 0 /* Enable the uart driver */
#define EZ_OSAL 0 /* Enable operating system abstract layer */
#define EZ_OSAL_USE_STATIC 0 /* Enable operating system using static allocation */

/* Configure HAL driver */
#define EZ_HAL_ECHO 0 /* Enable HAL echo driver */
#define EZ_HAL_UART 0 /* Enable HAL uart driver */

/* Configure 3rd Party */
#define LITTLE_FS 0 /* Enable littlefs library */
#define THREADX 0 /* Enable threadx RTOS */
#define FREERTOS 0 /* Enable FreeRTOS */

#endif
