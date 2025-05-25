# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_lwip_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_lwip_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_lwip_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_lwip_lib
    PRIVATE
        ez_lwip.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_lwip_lib
    PUBLIC
        LWIP_ENABLE=$<BOOL:${ENABLE_LWIP}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_lwip_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_lwip_lib
    PUBLIC
        lwip
    PRIVATE
        ez_utilities_lib
    INTERFACE
        # Please add interface libraries
)

# End of file
