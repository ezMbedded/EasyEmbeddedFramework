# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_target
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for building linux target
# ----------------------------------------------------------------------------

add_executable(ez_target)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_target build files")
message(STATUS "**********************************************************")


# Include --------------------------------------------------------------------
include(features.cmake)


# Source files ---------------------------------------------------------------
target_sources(ez_target
    PRIVATE
        main.c
        app_task_worker/ez_app_task_worker.c
        app_task_worker/ez_worker1.c
        app_task_worker/ez_worker2.c
        app_osal/ez_app_osal.c
        app_cli/ez_app_cli.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_target
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_target
    PUBLIC
        app_osal/
        app_task_worker/
        app_cli/
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)

# Config RTOS -----------------------------------------------------------------
set(FREERTOS_HEAP "4" CACHE INTERNAL "")
set(FREERTOS_PORT "GCC_POSIX" CACHE INTERNAL "")

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
INTERFACE
    .
)
target_compile_definitions(freertos_config
INTERFACE
    projCOVERAGE_TEST=0
)

# Link libraries -------------------------------------------------------------
target_link_libraries(ez_target
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
    INTERFACE
        # Please add interface libraries
)

# End of file
