# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_gpio_test
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_executable(ez_gpio_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_osez_gpio_testal_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_gpio_test
    PRIVATE
        unittest_ez_gpio.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_gpio_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_gpio_test
    PUBLIC
        ${CURRENT_DIR}
        ${SDK_ROOT_DIR}/ezmsdk
        ${FFF_PATH}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)

# Link libraries -------------------------------------------------------------
target_link_libraries(ez_gpio_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
        Catch2::Catch2WithMain
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_gpio_test
    COMMAND ez_gpio_test
)

catch_discover_tests(ez_gpio_test)

# End of file
