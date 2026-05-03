# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: easy_embedded_lib
# Description: CMake file to genreate build files for easy_embedded library
# ----------------------------------------------------------------------------

add_library(easy_embedded_lib INTERFACE)

message(STATUS "**********************************************************")
message(STATUS "* Generating easy_embedded library build files")
message(STATUS "**********************************************************")

# GCC/Clang quality and security flags for framework targets only
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wconversion
        -Wshadow
        -Wformat=2
        -Wnull-dereference
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
    )

    add_compile_definitions(
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:_FORTIFY_SOURCE=2>
    )
endif()

option(BUILD_WITH_CMAKE "code base is built using CMake. Do not turn this OFF" ON)

# Include --------------------------------------------------------------------
# Unused


# Sub directory --------------------------------------------------------------
add_subdirectory(service)
add_subdirectory(middlewares)
add_subdirectory(utilities)
add_subdirectory(hal)


# Source files ---------------------------------------------------------------



# Definitions ----------------------------------------------------------------
target_compile_definitions(easy_embedded_lib
    INTERFACE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(easy_embedded_lib
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        ${FRAMEWORK_ROOT_DIR}
        ${CMAKE_SOURCE_DIR}/inc
)


# Link libraries -------------------------------------------------------------
target_link_libraries(easy_embedded_lib
    INTERFACE
        ez_service_lib
        ez_middlewares_lib
        ez_utilities_lib
        ez_hal_lib
)

# End of file
