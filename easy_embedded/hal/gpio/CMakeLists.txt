# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_gpio_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_gpio_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_gpio_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_gpio_lib
    PRIVATE
        # Please add source files here
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_gpio_lib
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_gpio_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_gpio_lib
    PUBLIC
        # Please add public libraries
    PRIVATE
        # Please add private libraries
    INTERFACE
        # Please add interface libraries
)

# End of file
