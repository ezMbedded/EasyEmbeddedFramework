# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_task_worker_lib
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for task worker component
# ----------------------------------------------------------------------------

add_library(ez_task_worker_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_task_worker_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)


# Source files ---------------------------------------------------------------
target_sources(ez_task_worker_lib
    PRIVATE
        ez_task_worker.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_task_worker_lib
    PUBLIC
        EZ_TASK_WORKER=$<BOOL:${ENABLE_EZ_TASK_WORKER}>
        EZ_THREADX_PORT=$<BOOL:${ENABLE_THREADX}>
        EZ_FREERTOS_PORT=$<BOOL:${ENABLE_FREERTOS}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_task_worker_lib
    PUBLIC
        ${FRAMEWORK_ROOT_DIR}/inc/service/task_worker
    PRIVATE
        #
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_task_worker_lib
    PUBLIC
        $<$<BOOL:${ENABLE_EZ_OSAL}>:ez_osal_lib>
        $<$<BOOL:${ENABLE_FREERTOS}>:freertos_kernel>
        $<$<BOOL:${ENABLE_THREADX}>:threadx>
    PRIVATE
        ez_utilities_lib
    INTERFACE
)

# End of file
