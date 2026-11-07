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


set(FRAMEWORK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)


# Dependency -----------------------------------------------------------------
# All the HAL modules (uart, gpio, i2c, spi) include ez_driver_def.h, which is
# only provided when EZ_DRIVER is 1. Enable the driver module automatically
# when any of them is enabled. This is a local override and does not change
# the user's cache value.
if((ENABLE_EZ_UART OR ENABLE_EZ_GPIO OR ENABLE_EZ_I2C OR ENABLE_EZ_SPI)
   AND NOT ENABLE_EZ_HAL_DRIVER)
    message(STATUS "* HAL modules need the driver module: ENABLE_EZ_HAL_DRIVER forced to ON")
    set(ENABLE_EZ_HAL_DRIVER ON)
endif()


# Source files ---------------------------------------------------------------
target_sources(ez_hal_lib
    PRIVATE
        $<$<BOOL:${ENABLE_EZ_UART}>:uart/ez_uart.c>
        $<$<BOOL:${ENABLE_EZ_GPIO}>:gpio/ez_gpio.c>
        $<$<BOOL:${ENABLE_EZ_I2C}>:i2c/ez_i2c.c>
        $<$<BOOL:${ENABLE_EZ_SPI}>:spi/ez_spi.c>
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_hal_lib
    PUBLIC
        EZ_HAL=$<BOOL:${ENABLE_EZ_HAL}>
        EZ_DRIVER=$<BOOL:${ENABLE_EZ_HAL_DRIVER}>
        EZ_UART=$<BOOL:${ENABLE_EZ_UART}>
        EZ_GPIO=$<BOOL:${ENABLE_EZ_GPIO}>
        EZ_I2C=$<BOOL:${ENABLE_EZ_I2C}>
        EZ_SPI=$<BOOL:${ENABLE_EZ_SPI}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_hal_lib
    PUBLIC
        ${FRAMEWORK_ROOT_DIR}/inc/hal
        ${FRAMEWORK_ROOT_DIR}/inc/hal/uart
        ${FRAMEWORK_ROOT_DIR}/inc/hal/gpio
        ${FRAMEWORK_ROOT_DIR}/inc/hal/i2c
        ${FRAMEWORK_ROOT_DIR}/inc/hal/spi
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_hal_lib
    PRIVATE
        ez_utilities_lib
)

# End of file
