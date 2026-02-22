cmake_minimum_required(VERSION 3.25)

project(EzEmbeddedFramework
    VERSION 1.0.0
    LANGUAGES C CXX
    DESCRIPTION "Easy Embedded Software Framework"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_NUMBER 0)


# Configure a header file with version + build number
configure_file(
    cmake/ez_version.h.in
    inc/ez_version.h
)

#Only build test, targets, and extern when being build as a project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    add_subdirectory(${TARGET_PATH})
    add_subdirectory(extern)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_custom_target(doxygen
    COMMAND doxygen Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(coverage
    COMMAND cd ${CMAKE_SOURCE_DIR} && cmake --build --preset=linux_test_debug
    COMMAND cd ${CMAKE_SOURCE_DIR} && ctest --preset=linux_test_debug
    COMMAND cd ${CMAKE_SOURCE_DIR} && rm -rf coverage && mkdir coverage
    COMMAND cd ${CMAKE_SOURCE_DIR} && gcovr -r . --filter=easy_embedded --html --html-details -o coverage/coverage.html)

add_custom_target(docs
    COMMAND sphinx-build -c ${CMAKE_SOURCE_DIR}/docs/ -v ${CMAKE_SOURCE_DIR}/docs/ ${CMAKE_SOURCE_DIR}/docs/_build/)
