# ----------------------------------------------------------------------------
# Author: Hai Nguyen
# Name: ez_utilities_lib
# License: This file is published under the license described in LICENSE.md
# Description: PLEASE ADD TEXT HERE
# ----------------------------------------------------------------------------

add_library(ez_utilities_lib STATIC)

message(STATUS "**********************************************************")
message(STATUS "* Generating ez_utilities library build files")
message(STATUS "**********************************************************")

set(FRAMEWORK_ROOT_DIR ${CMAKE_SOURCE_DIR}/easy_embedded)


# Source files ---------------------------------------------------------------
target_sources(ez_utilities_lib
    PRIVATE
        assert/ez_assert.c
        hexdump/ez_hexdump.c
        linked_list/ez_linked_list.c
        logging/ez_logging.c
        ring_buffer/ez_ring_buffer.c
        static_alloc/ez_static_alloc.c
        system_error/ez_system_error.c
        queue/ez_queue.c
)


# Definitions ----------------------------------------------------------------
target_compile_definitions(ez_utilities_lib
    PUBLIC
        EZ_LOGGING=$<BOOL:${ENABLE_EZ_LOGGING}>
        EZ_LOGGING_COLOR=$<BOOL:${ENABLE_EZ_LOGGING_COLOR}>
        EZ_LINKEDLIST=$<BOOL:${ENABLE_EZ_LINKEDLIST}>
        EZ_HEXDUMP=$<BOOL:${ENABLE_EZ_HEXDUMP}>
        EZ_RING_BUFFER=$<BOOL:${ENABLE_EZ_RING_BUFFER}>
        EZ_ASSERT=$<BOOL:${ENABLE_EZ_ASSERT}>
        EZ_STATIC_ALLOC=$<BOOL:${ENABLE_EZ_STATIC_ALLOC}>
        EZ_SYS_ERROR=$<BOOL:${ENABLE_EZ_SYS_ERROR}>
        EZ_QUEUE=$<BOOL:${ENABLE_EZ_QUEUE}>
    PRIVATE
        EZ_BUILD_WITH_CMAKE=$<BOOL:${BUILD_WITH_CMAKE}>
)


# Include directory -----------------------------------------------------------
target_include_directories(ez_utilities_lib
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/endian
        ${CMAKE_CURRENT_LIST_DIR}/assert
        ${CMAKE_CURRENT_LIST_DIR}/hexdump
        ${CMAKE_CURRENT_LIST_DIR}/linked_list
        ${CMAKE_CURRENT_LIST_DIR}/logging
        ${CMAKE_CURRENT_LIST_DIR}/ring_buffer
        ${CMAKE_CURRENT_LIST_DIR}/static_alloc
        ${CMAKE_CURRENT_LIST_DIR}/system_error
        ${CMAKE_CURRENT_LIST_DIR}/queue
        ${CMAKE_CURRENT_LIST_DIR}
)


# Link libraries -------------------------------------------------------------
target_link_libraries(ez_utilities_lib
    PUBLIC
        # Unused
    PRIVATE
        # Unused
    INTERFACE
        # Unused
)


# End of file
