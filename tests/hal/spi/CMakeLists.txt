# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_spi_test
# License: This file is published under the license described in LICENSE.md
# Description: Unit tests for the SPI HAL component
# ----------------------------------------------------------------------------

add_executable(ez_spi_test)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_spi_test build files")
message(STATUS "**********************************************************")


# Source files ---------------------------------------------------------------
target_sources(ez_spi_test
    PRIVATE
        unittest_ez_spi.cpp
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_spi_test
    PUBLIC
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_spi_test
    PUBLIC
        ${CURRENT_DIR}
        ${FFF_PATH}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)

# Link libraries -------------------------------------------------------------
target_link_libraries(ez_spi_test
    PUBLIC
        # Please add public libraries
    PRIVATE
        easy_embedded_lib
        Catch2::Catch2WithMain
    INTERFACE
        # Please add interface libraries
)

add_test(NAME ez_spi_test
    COMMAND ez_spi_test
)

catch_discover_tests(ez_spi_test)

# End of file