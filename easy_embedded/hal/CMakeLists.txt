# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_hal_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_hal_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_hal_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_hal_lib
    PRIVATE
        ez_hal.c
        $<$<BOOL:${ENABLE_EZ_UART}>:uart/ez_uart.c>
        $<$<BOOL:${ENABLE_EZ_GPIO}>:gpio/ez_gpio.c>
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_hal_lib
    PUBLIC
        EZ_HAL=$<BOOL:${ENABLE_EZ_HAL}>
        EZ_DRIVER=$<BOOL:${ENABLE_EZ_HAL_DRIVER}>
        EZ_UART=$<BOOL:${ENABLE_EZ_UART}>
        EZ_GPIO=$<BOOL:${ENABLE_EZ_GPIO}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_hal_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/uart
        ${CMAKE_CURRENT_LIST_DIR}/gpio
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_hal_lib
    PUBLIC
        # Please add public libraries
    PRIVATE
        ez_utilities_lib
        ez_service_lib
    INTERFACE
        # Please add interface libraries
)

# End of file
