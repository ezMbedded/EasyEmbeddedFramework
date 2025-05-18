# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# License: This file is published under the license described in LICENSE.md
# Description: CMake file to build unit tests
# ----------------------------------------------------------------------------

message(STATUS "************************************")
message(STATUS "* Building unit tests")
message(STATUS "************************************")


# activate test depend on the activated module
if(ENABLE_EZ_OSAL)
    add_subdirectory(middlewares/osal)
endif()

if(ENABLE_DATA_MODEL)
    add_subdirectory(app/data_model)
endif()

if(ENABLE_EZ_EVENT_NOTIFIER)
    add_subdirectory(service/event_notifier)
endif()

if(ENABLE_EZ_TASK_WORKER AND NOT ENABLE_THREADX AND NOT ENABLE_FREERTOS)
    add_subdirectory(service/task_worker)
endif()

if(ENABLE_EZ_STATE_MACHINE)
    add_subdirectory(service/state_machine)
endif()

if(ENABLE_EZ_KERNEL)
    add_subdirectory(service/kernel)
endif()

if(ENABLE_EZ_HAL_ECHO)
    add_subdirectory(service/driver/hal_echo)
endif()

if(ENABLE_EZ_UART)
    add_subdirectory(service/driver/uart)
endif()

if(ENABLE_EZ_QUEUE)
    add_subdirectory(utilities/queue)
endif()

if(ENABLE_EZ_LINKEDLIST)
    add_subdirectory(utilities/linked_list)
endif()

if(ENABLE_EZ_STATIC_ALLOC)
    add_subdirectory(utilities/static_alloc)
endif()

if(ENABLE_EZ_RING_BUFFER)
    add_subdirectory(utilities/ring_buffer)
endif()

if(ENABLE_EZ_GPIO)
    add_subdirectory(hal/gpio)
endif()

# End of file