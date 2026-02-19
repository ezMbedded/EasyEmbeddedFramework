cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

#Adding Unity test framework -------------------------------------------------
if(BUILD_TESTING)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY    https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG           v2.5.2
        #SOURCE_DIR        ${CMAKE_SOURCE_DIR}/extern/unity
    )

    FetchContent_GetProperties(unity)
    if(NOT unity_POPULATED)
        message(STATUS "Unity framework is not available, downloading...")
        
        FetchContent_Populate(unity)
        SET(UNITY_EXTENSION_FIXTURE ON CACHE BOOL "Build Unity with extension features")
        add_subdirectory(
            ${unity_SOURCE_DIR}
            ${unity_BINARY_DIR}
            EXCLUDE_FROM_ALL)
        
        message(STATUS "Download completed")
    endif()
endif()

#Adding littlefs file system -------------------------------------------------
if(ENABLE_LITTLE_FS)
    FetchContent_Declare(
        littlefs
        GIT_REPOSITORY    https://github.com/littlefs-project/littlefs.git
        GIT_TAG           v2.7.0
        #SOURCE_DIR        ${CMAKE_SOURCE_DIR}/extern/littlefs
    )

    FetchContent_GetProperties(littlefs)
    if(NOT littlefs_POPULATED)
        message(STATUS "littlefs is not available, downloading...")
        FetchContent_Populate(littlefs)
        message(STATUS "Download completed")
    endif()

    add_subdirectory(littlefs)
endif()

#Adding threadx rtos ---------------------------------------------------------
if(ENABLE_THREADX)
    FetchContent_Declare(
        threadx
        GIT_REPOSITORY    https://github.com/eclipse-threadx/threadx.git
        GIT_TAG           v6.4.1_rel
        SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/threadx
    )

    FetchContent_GetProperties(threadx)
    if(NOT threadx)
        message(STATUS "threadx is not available, downloading...")
        FetchContent_Populate(threadx)
        message(STATUS "Download completed")
    endif()

    add_subdirectory(threadx)
endif()

#Adding FreeRTOS -------------------------------------------------------------
if(ENABLE_FREERTOS)
    FetchContent_Declare(
        freertos_kernel
        GIT_REPOSITORY    https://github.com/FreeRTOS/FreeRTOS-Kernel.git
        GIT_TAG           V11.0.1
        SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/freertos
    )

    FetchContent_GetProperties(freertos_kernel)
    if(NOT freertos_kernel)
        message(STATUS "freertos is not available, downloading...")
        FetchContent_Populate(freertos_kernel)
        message(STATUS "freertos completed")
    endif()

    add_subdirectory(${CMAKE_SOURCE_DIR}/extern/freertos)
endif()

#Adding FFF -------------------------------------------------------------
if(BUILD_TESTING)
    FetchContent_Declare(
        fff
        GIT_REPOSITORY    https://github.com/meekrosoft/fff.git
        GIT_TAG           v1.1
        SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/fff
    )

    FetchContent_GetProperties(fff)
    if(NOT fff_POPULATED)
        message(STATUS "FFF framework is not available, downloading...")

        FetchContent_Populate(fff)
        SET(FFF_GENERATE OFF CACHE BOOL "")
        SET(FFF_UNIT_TESTING OFF CACHE BOOL "")

        message(STATUS "Download completed")
        SET(FFF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/fff CACHE STRING "Path to FFF framework")
    endif()
endif()

#Adding Catch2 -------------------------------------------------------------
if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY    https://github.com/catchorg/Catch2.git
        GIT_TAG           v3.8.0
    )

    FetchContent_GetProperties(Catch2)
    if(NOT Catch2_POPULATED)
        message(STATUS "Catch2 framework is not available, downloading...")
        FetchContent_MakeAvailable(Catch2)
        message(STATUS "Download completed")
    endif()

    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
    include(Catch)
endif()
