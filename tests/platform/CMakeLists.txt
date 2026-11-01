# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: unittest_ez_platform
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_executable(ez_platform_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_platform_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_platform_test
    PRIVATE
        unittest_ez_platform.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_platform_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_platform_test
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
target_link_libraries(ez_platform_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
        Catch2::Catch2WithMain
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_platform_test
    COMMAND ez_platform_test
)

catch_discover_tests(ez_platform_test)
# End of file
