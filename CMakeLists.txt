cmake_minimum_required(VERSION 3.5)
project(graphics-apps)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake CACHE STRING
    "CMake module path." FORCE)

option(RPI_PLATFORM "Build for RPI" OFF)
option(SYNAPTICS_PLATFORM "Build for Synaptics" OFF)
option(X86_64_PLATFORM "Build for x86_64" ON)

if (RPI_PLATFORM)
    message(STATUS "Selecting RPI platform")
    set(CMAKE_FRAMEWORK_PATH /opt/rpi/usr)
#    set(CMAKE_TOOLCHAIN_FILE /home/rene/armv8-linux-gnueabihf.cmake)
elseif (SYNAPTICS_PLATFORM)
    message(STATUS "Selecting Synaptics platform")
    set(CMAKE_FRAMEWORK_PATH /opt/synaptics/usr)
#    set(CMAKE_TOOLCHAIN_FILE /home/rene/armv8-linux-gnueabihf.cmake)
elseif (X86_64_PLATFORM)
    message(STATUS "Selecting Desktop x86_64 platform")
    set(CMAKE_FRAMEWORK_PATH /usr)
#    set(CMAKE_TOOLCHAIN_FILE /home/rene/x86_64-linux-gnu.cmake)
else()
    message(ERROR "Undefined platform")
endif()

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
message(STATUS "System name:                  " ${CMAKE_SYSTEM_NAME})
message(STATUS "System processor:             " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "C compiler:                   " ${CMAKE_C_COMPILER})
message(STATUS "C++ compiler:                 " ${CMAKE_CXX_COMPILER})
message(STATUS "Cross-compiling:              " ${CMAKE_CROSSCOMPILING})

add_subdirectory(egl-test)
add_subdirectory(westeros-test)
add_subdirectory(westeros)
