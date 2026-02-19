# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_event_notifier_test
# License: This file is published under the license described in LICENSE.md
# Description: CMake file for event notifier unit test
# ----------------------------------------------------------------------------

add_executable(ez_cli_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_cli_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_cli_test
    PRIVATE
        unittest_ez_cli.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_cli_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_cli_test
    PUBLIC
        # Please add private folders here
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_cli_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        unity
        easy_embedded_lib
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_cli_test
    COMMAND ez_cli_test
)

# End of file
