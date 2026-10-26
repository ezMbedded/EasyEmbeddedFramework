# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_state_machine_test
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

if(ENABLE_EZ_STATE_MACHINE)
add_executable(ez_state_machine_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_state_machine_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_state_machine_test
    PRIVATE
        unittest_ez_state_machine.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_state_machine_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_state_machine_test
    PUBLIC
        # Please add private folders here
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_state_machine_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        unity
        easy_embedded_lib
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_state_machine_test
    COMMAND ez_state_machine_test
)
endif()


# ----------------------------------------------------------------------------
# State machine v2 (Catch2 + fff)
# ----------------------------------------------------------------------------
if(ENABLE_EZ_STATE_MACHINE_V2)
    add_executable(ez_state_machine_v2_test)

    message(STATUS "**********************************************************")
    message(STATUS "* Generating ez_state_machine_v2_test build files")
    message(STATUS "**********************************************************")

    target_sources(ez_state_machine_v2_test
        PRIVATE
            unittest_ez_state_machine_v2.cpp
    )

    target_include_directories(ez_state_machine_v2_test
        PUBLIC
            ${FFF_PATH}
    )

    target_link_libraries(ez_state_machine_v2_test
        PRIVATE
            easy_embedded_lib
            Catch2::Catch2WithMain
    )

    add_test(NAME ez_state_machine_v2_test
        COMMAND ez_state_machine_v2_test
    )

    catch_discover_tests(ez_state_machine_v2_test)
endif()

# End of file
