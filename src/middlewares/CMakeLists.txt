# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_middlewares_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_middlewares_lib INTERFACE)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_middlewares_lib library build files")
message(STATUS "**********************************************************")

# Inlcude subfolders-----------------------------------------------------------
if(ENABLE_EZ_OSAL)
    add_subdirectory(osal)
endif()


# Source files ---------------------------------------------------------------


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_middlewares_lib
    INTERFACE
        EZ_MIDDLEWARES=$<BOOL:${ENABLE_EZ_MIDDLEWARES}>
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>

)


# Include directory -----------------------------------------------------------
target_include_directories(ez_middlewares_lib
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_middlewares_lib
    INTERFACE
        $<$<BOOL:${ENABLE_EZ_OSAL}>:ez_osal_lib>
        ez_utilities_lib
)

# End of file
