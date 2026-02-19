# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_ipc_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_ipc_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_ipc_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_ipc_lib
    PRIVATE
        ez_ipc.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_ipc_lib
    PUBLIC
        EZ_IPC=$<BOOL:${ENABLE_EZ_IPC}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_ipc_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/service/ipc
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_ipc_lib
    PUBLIC
        # Please add public libraries
    PRIVATE
        ez_utilities_lib
    INTERFACE
        # Please add interface libraries
)

# End of file
