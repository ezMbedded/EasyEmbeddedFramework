# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_osal_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_osal_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_osal_lib library build files")
message(STATUS "**********************************************************")


set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_osal_lib
    PRIVATE
        ez_osal.c
        $<$<BOOL:${ENABLE_FREERTOS}>:ez_osal_freertos.c>
        $<$<BOOL:${ENABLE_THREADX}>:ez_osal_threadx.c>
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_osal_lib
    PUBLIC
        EZ_OSAL=$<BOOL:${ENABLE_EZ_OSAL}>
        EZ_FREERTOS_PORT=$<BOOL:${ENABLE_FREERTOS}>
        EZ_OSAL_USE_STATIC=$<BOOL:${ENABLE_EZ_OSAL_USE_STATIC}>
        EZ_THREADX_PORT=$<BOOL:${ENABLE_THREADX}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_osal_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/middlewares/osal
    PRIVATE
        # Please add private folders here
    INTERFACE
        # Please add interface folders here
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_osal_lib
    PUBLIC
        # Please add public libraries
    PRIVATE
        ez_utilities_lib
        $<$<BOOL:${ENABLE_FREERTOS}>:freertos_kernel>
        $<$<BOOL:${ENABLE_THREADX}>:threadx>
    INTERFACE
        # Please add interface libraries
)

# End of file
