option(ENABLE_EZ_SERVICE           "Enable HAL layer"                   ON)
option(ENABLE_EZ_MIDDLEWARES       "Enable Middlewares layer"           ON)
option(ENABLE_EZ_HAL               "Enable HAL layer"                   ON)

# Configure Utilities modules
option(ENABLE_EZ_LOGGING        "Enable logging feature"                ON)
option(ENABLE_EZ_LOGGING_COLOR  "Enable logging using color feature"    OFF)
option(ENABLE_EZ_LINKEDLIST     "Enable linked list feature"            ON)
option(ENABLE_EZ_HEXDUMP        "Enable hexdump feature"                ON)
option(ENABLE_EZ_RING_BUFFER    "Enable ring buffer feature"            ON)
option(ENABLE_EZ_ASSERT         "Enable assert feature"                 OFF)
option(ENABLE_EZ_STATIC_ALLOC   "Enable static allocation feature"      ON)
option(ENABLE_EZ_SYS_ERROR      "Enable system error feature"           ON)
option(ENABLE_EZ_QUEUE          "Enable queue feature"                  ON)

# Configure Service modules
option(ENABLE_EZ_EVENT_NOTIFIER    "Enable the Event Notifier module"       OFF)
option(ENABLE_EZ_KERNEL            "Enable the Kernel service"              OFF)
option(ENABLE_EZ_TASK_WORKER       "Enable the task worker"                 OFF)
option(ENABLE_EZ_STATE_MACHINE     "Enable state machine"                   OFF)

# Configure application framework
option(ENABLE_DATA_MODEL        "Enable the Event Notifier module"          OFF)
option(ENABLE_EZ_CLI            "Enable command line interface"             OFF)
option(ENABLE_EZ_RPC            "Enable remote procedure call"              OFF)
option(ENABLE_EZ_IPC            "Enable inter process communication"        OFF)
option(ENABLE_EZ_HAL_DRIVER     "Enable the Driver module"                  OFF)
option(ENABLE_EZ_UART           "Enable the uart driver"                    OFF)

option(ENABLE_EZ_OSAL               "Enable operating system abstract layer"            OFF)
option(ENABLE_EZ_OSAL_USE_STATIC    "Enable operating system using static allocation"   OFF)

# Configure HAL driver
option(ENABLE_EZ_HAL_ECHO       "Enable HAL echo driver"                    OFF)
option(ENABLE_EZ_HAL_UART       "Enable HAL uart driver"                    OFF)

# Configure 3rd Party
option(ENABLE_LITTLE_FS         "Enable littlefs library"                   OFF)
option(ENABLE_THREADX           "Enable threadx RTOS"                       OFF)
option(ENABLE_FREERTOS          "Enable FreeRTOS"                           OFF)