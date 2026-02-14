
option(ENABLE_EZ_SERVICE           "Enable HAL layer"                   ON)
option(ENABLE_EZ_BSP               "Enable BSP layer"                   ON)
option(ENABLE_EZ_MIDDLEWARES       "Enable Middlewares layer"           ON)
option(ENABLE_EZ_HAL               "Enable HAL layer"                   ON)


# Configure Utilities modules
option(ENABLE_EZ_LOGGING        "Enable logging feature"                ON)
option(ENABLE_EZ_LOGGING_COLOR  "Enable logging using color feature"    ON)
option(ENABLE_EZ_LINKEDLIST     "Enable linked list feature"            ON)
option(ENABLE_EZ_HEXDUMP        "Enable hexdump feature"                ON)
option(ENABLE_EZ_RING_BUFFER    "Enable ring buffer feature"            ON)
option(ENABLE_EZ_ASSERT         "Enable assert feature"                 ON)
option(ENABLE_EZ_STATIC_ALLOC   "Enable static allocation feature"      ON)
option(ENABLE_EZ_SYS_ERROR      "Enable system error feature"           ON)
option(ENABLE_EZ_QUEUE          "Enable queue feature"                  ON)

# Configure Service modules
option(ENABLE_EZ_EVENT_BUS         "Enable the Event Bus module"            ON)
option(ENABLE_EZ_KERNEL            "Enable the Kernel service"              OFF)
option(ENABLE_EZ_TASK_WORKER       "Enable the task worker"                 ON)
option(ENABLE_EZ_STATE_MACHINE     "Enable state machine"                   ON)

# Configure application framework
option(ENABLE_DATA_MODEL        "Enable the Event Notifier module"          ON)
option(ENABLE_EZ_CLI            "Enable command line interface"             ON)
option(ENABLE_EZ_RPC            "Enable remote procedure call"              ON)
option(ENABLE_EZ_IPC            "Enable inter process communication"        ON)
option(ENABLE_EZ_HAL_DRIVER     "Enable the Driver module"                  ON)
option(ENABLE_EZ_UART           "Enable the uart driver"                    ON)
option(ENABLE_EZ_I2C            "Enable the i2c driver"                     ON)
option(ENABLE_EZ_SPI            "Enable the spi driver"                     ON)

option(ENABLE_EZ_OSAL           "Enable operating system abstract layer"                ON)
option(ENABLE_EZ_OSAL_USE_STATIC    "Enable operating system using static allocation"   ON)

# Configure HAL driver
option(ENABLE_EZ_HAL_ECHO       "Enable HAL echo driver"                    OFF)
option(ENABLE_EZ_HAL_UART       "Enable HAL uart driver"                    OFF)

# Configure 3rd Party
option(ENABLE_LITTLE_FS         "Enable littlefs library"                   OFF)
option(ENABLE_THREADX           "Enable threadx RTOS"                       ON)
option(ENABLE_FREERTOS          "Enable FreeRTOS"                           OFF)