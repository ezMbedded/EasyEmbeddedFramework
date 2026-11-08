# Download the Raspberry Pi Pico SDK (with its submodules) into extern/pico-sdk.
# Must be included BEFORE project(): the preset's toolchain file lives inside the SDK.
include(FetchContent)

set(PICO_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../extern/pico-sdk)
cmake_path(NORMAL_PATH PICO_SDK_DIR)

if(NOT EXISTS ${PICO_SDK_DIR}/pico_sdk_init.cmake)
    message(STATUS "pico-sdk is not available, downloading (with submodules)...")
    FetchContent_Declare(
        pico_sdk
        GIT_REPOSITORY          https://github.com/raspberrypi/pico-sdk.git
        GIT_TAG                 2.3.1
        GIT_SHALLOW             TRUE
        GIT_SUBMODULES_RECURSE  FALSE   # same as the SDK README: top-level submodules only
        SOURCE_DIR              ${PICO_SDK_DIR}
    )
    FetchContent_GetProperties(pico_sdk)
    if(NOT pico_sdk_POPULATED)
        FetchContent_Populate(pico_sdk)
    endif()
    message(STATUS "pico-sdk download completed")
endif()
