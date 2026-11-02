# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_target
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for building linux target
# ----------------------------------------------------------------------------

add_executable(ez_linux_platform_target)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_linux_platform_target build files")
message(STATUS "**********************************************************")


# Include --------------------------------------------------------------------
include(features.cmake)

# Source files ---------------------------------------------------------------
target_sources(ez_linux_platform_target
    PRIVATE
        main.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_linux_platform_target
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_linux_platform_target
    PUBLIC
        # Please add private folders here
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_linux_platform_target
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
    INTERFACE
        # Please add interface libraries
)

# End of file
