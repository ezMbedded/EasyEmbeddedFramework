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
