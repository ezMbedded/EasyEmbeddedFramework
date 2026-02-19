# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_event_bus_lib
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for event bus component
# ----------------------------------------------------------------------------

add_library(ez_event_bus_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_event_bus library build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_event_bus_lib
    PRIVATE
        ez_event_bus.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_event_bus_lib
    PUBLIC
        EZ_EVENT_BUS=$<BOOL:${ENABLE_EZ_EVENT_BUS}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_event_bus_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/service/event_bus
    PRIVATE
        # Unused
    INTERFACE
        # Unused
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_event_bus_lib
    PUBLIC
        ez_utilities_lib
    PRIVATE
        # Unused
    INTERFACE
        # Unused
)


# End of file
