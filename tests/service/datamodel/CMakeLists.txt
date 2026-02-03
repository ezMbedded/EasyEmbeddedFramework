# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: unittest_ez_datamodel
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_executable(ez_datamodel_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_datamodel_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_datamodel_test
    PRIVATE
        unittest_ez_datamodel.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_datamodel_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_datamodel_test
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
target_link_libraries(ez_datamodel_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
        Catch2::Catch2WithMain
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_datamodel_test
    COMMAND ez_datamodel_test
)

catch_discover_tests(ez_datamodel_test)
# End of file
