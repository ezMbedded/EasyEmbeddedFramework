# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: easy_embedded_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(easy_embedded_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating easy_embedded library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)
option(BUILD_WITH_CMAKE "code base is built using CMake. Do not turn this OFF" ON)

# Include --------------------------------------------------------------------
# Unused


# Sub directory --------------------------------------------------------------
add_subdirectory(service)
add_subdirectory(middlewares)
add_subdirectory(utilities)
add_subdirectory(hal)


# Source files ---------------------------------------------------------------
target_sources(easy_embedded_lib
    PRIVATE
        ez_easy_embedded.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(easy_embedded_lib
    PUBLIC
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
    PRIVATE
        # Please add definitions here
)


# Include directory -----------------------------------------------------------
target_include_directories(easy_embedded_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${FRAMEWORK_ROOT_DIR}
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(easy_embedded_lib
    PUBLIC
        ez_service_lib
        ez_middlewares_lib
        ez_utilities_lib
        ez_hal_lib
        $<$<BOOL:${ENABLE_LITTLE_FS}>:littlefs>
    PRIVATE
        # Please add private libraries
    INTERFACE
        # Please add interface libraries
)

# End of file
