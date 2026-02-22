# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: data_model_lib
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for data model component
# ----------------------------------------------------------------------------

add_library(ez_data_model_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating data_model library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)


# Source files ---------------------------------------------------------------
target_sources(ez_data_model_lib
    PRIVATE
        ez_data_model.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_data_model_lib
    PUBLIC
        DATA_MODEL=$<BOOL:${ENABLE_DATA_MODEL}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_data_model_lib
    PUBLIC
        ${FRAMEWORK_ROOT_DIR}/inc/service/data_model
    PRIVATE
        # Unused
    INTERFACE
        # Unused
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_data_model_lib
    PUBLIC
        # Unused
    PRIVATE
        ez_utilities_lib
        ez_event_bus_lib
    INTERFACE
        # Unused
)

# End of file
