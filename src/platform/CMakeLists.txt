# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_platform_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_platform_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_platform_lib library build files")
message(STATUS "**********************************************************")

set(FRAMEWORK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)


# Source files ---------------------------------------------------------------
target_sources(ez_platform_lib
    PRIVATE
        ez_platform.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_platform_lib
    PUBLIC
        EZ_PLATFORM=$<BOOL:${ENABLE_EZ_PLATFORM}>
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_platform_lib
    PUBLIC
        ${FRAMEWORK_ROOT_DIR}/inc/platform
        ${FRAMEWORK_ROOT_DIR}/inc/platform/task
        ${FRAMEWORK_ROOT_DIR}/inc/platform/sync
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_platform_lib
    PUBLIC
        ez_service_lib
    PRIVATE
        ez_utilities_lib
)

# End of file
