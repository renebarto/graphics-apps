cmake_minimum_required(VERSION 3.5)
project(hello-wayland)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake CACHE STRING
    "CMake module path." FORCE)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE   ON)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
    "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CONFIG_DIR "debug" CACHE STRING "Build config directory" FORCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CONFIG_DIR "release" CACHE STRING "Build config directory" FORCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
else()
    message(FATAL_ERROR "Invalid build type: " ${CMAKE_BUILD_TYPE})
endif()

set(BUILD_BASE_DIR ${CMAKE_SOURCE_DIR}/build CACHE STRING
    "Build root directory.")
set(CMAKE_BINARY_DIR ${BUILD_BASE_DIR}/${CONFIG_DIR} CACHE STRING
    "Build directory.")
message(STATUS "Build base directory:         " ${BUILD_BASE_DIR})
message(STATUS "Build config directory:       " ${CMAKE_BINARY_DIR})

set(OUTPUT_BASE_DIR ${CMAKE_SOURCE_DIR}/output)
message(STATUS "Output base directory:        " ${OUTPUT_BASE_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR}/lib)
message(STATUS "Static lib output directory:  " ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR}/bin)
message(STATUS "Dynamic lib output directory: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR}/bin)
message(STATUS "Executable output directory:  " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(PACKAGE_DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/download)
message(STATUS "Download directory:           " ${PACKAGE_DOWNLOAD_DIR})
set(PACKAGE_BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
message(STATUS "Build directory:              " ${PACKAGE_BUILD_DIR})
set(PACKAGE_STAMP_DIR ${CMAKE_SOURCE_DIR}/stamp)
message(STATUS "Stamp directory:              " ${PACKAGE_STAMP_DIR})
set(PACKAGE_STAGING_DIR ${CMAKE_SOURCE_DIR}/staging)

set(CMAKE_SYSROOT ${SYSROOT_PATH} CACHE PATH "System root directory" FORCE)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SYSROOT}/usr CACHE PATH "Install directory" FORCE)
message(STATUS "Staging directory:            " ${CMAKE_INSTALL_PREFIX})
message(STATUS "System root directory:        " ${CMAKE_SYSROOT})
message(STATUS "Install directory:            " ${CMAKE_SYSROOT}/usr)

add_subdirectory(hello-wayland)
add_subdirectory(weston-simple-egl)
