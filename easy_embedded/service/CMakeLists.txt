# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_service_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_service_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_service_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Inlcude subfolders-----------------------------------------------------------
if(ENABLE_EZ_EVENT_NOTIFIER)
    add_subdirectory(event_notifier)
endif()

if(ENABLE_EZ_TASK_WORKER)
    add_subdirectory(task_worker)
endif()

if(ENABLE_EZ_STATE_MACHINE)
    add_subdirectory(state_machine)
endif()

if(ENABLE_DATA_MODEL)
    add_subdirectory(data_model)
endif()

if(ENABLE_EZ_CLI)
    add_subdirectory(cli)
endif()

if(ENABLE_EZ_RPC)
    add_subdirectory(rpc)
endif()

if(ENABLE_EZ_IPC)
    add_subdirectory(ipc)
endif()


# Source files ---------------------------------------------------------------
target_sources(ez_service_lib
    PRIVATE
        ez_service.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_service_lib
    PUBLIC
        EZ_SERVICE=$<BOOL:${ENABLE_EZ_SERVICE}>
        DATA_MODEL=$<BOOL:${ENABLE_DATA_MODEL}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_service_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_service_lib
    PUBLIC
        $<$<BOOL:${ENABLE_EZ_EVENT_NOTIFIER}>:ez_event_notifier_lib>
        $<$<BOOL:${ENABLE_EZ_TASK_WORKER}>:ez_task_worker_lib>
        $<$<BOOL:${ENABLE_EZ_STATE_MACHINE}>:ez_state_machine_lib>
        $<$<BOOL:${ENABLE_DATA_MODEL}>:ez_data_model_lib>
        $<$<BOOL:${ENABLE_EZ_CLI}>:ez_cli_lib>
        $<$<BOOL:${ENABLE_EZ_RPC}>:ez_rpc_lib>
        $<$<BOOL:${ENABLE_EZ_IPC}>:ez_ipc_lib>
        ez_utilities_lib
    PRIVATE
        # Please add private libraries
    INTERFACE
        # Please add interface libraries
)

# End of file
