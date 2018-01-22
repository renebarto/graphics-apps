# - Try to Find Wayland
#
# Copyright (C) 2017 Rene Barto
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Will be defined:
# WAYLAND_FOUND
# WAYLAND_CLIENT_FOUND
# WAYLAND_SERVER_FOUND
# WAYLAND_CURSOR_FOUND
# WAYLAND_CLIENT_INCLUDE_DIR
# WAYLAND_SERVER_INCLUDE_DIR
# WAYLAND_CURSOr_INCLUDE_DIR
# WAYLAND_CLIENT_LIBRARIES
# WAYLAND_SERVER_LIBRARIES
# WAYLAND_CURSOR_LIBRARIES
#

include(dump_all_vars)

find_package(PkgConfig)
pkg_check_modules(PC_WAYLAND_CLIENT QUIET wayland-client)
pkg_check_modules(PC_WAYLAND_SERVER QUIET wayland-server)
pkg_check_modules(PC_WAYLAND_CURSOR QUIET wayland-cursor)
pkg_check_modules(PC_WAYLAND_SCANNER QUIET wayland-scanner)

if(PC_WAYLAND_CLIENT_FOUND)
    set(WAYLAND_CLIENT_FOUND_TEXT "Found")
else()
    set(WAYLAND_CLIENT_FOUND_TEXT "Not found")
endif()

if(PC_WAYLAND_SERVER_FOUND)
    set(WAYLAND_SERVER_FOUND_TEXT "Found")
else()
    set(WAYLAND_SERVER_FOUND_TEXT "Not found")
endif()

if(PC_WAYLAND_CURSOR_FOUND)
    set(WAYLAND_CURSOR_FOUND "Found")
else()
    set(WAYLAND_CURSOR_FOUND "Not found")
endif()

if(PC_WAYLAND_SCANNER_FOUND)
    set(WAYLAND_SCANNER_FOUND "Found")
else()
    set(WAYLAND_SCANNER_FOUND "Not found")
endif()

message(STATUS "wayland-client:               ${WAYLAND_CLIENT_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_WAYLAND_CLIENT_VERSION}")
message(STATUS "  cflags:                     ${PC_WAYLAND_CLIENT_CFLAGS}")
message(STATUS "  cflags other:               ${PC_WAYLAND_CLIENT_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_WAYLAND_CLIENT_INCLUDE_DIRS} ${PC_WAYLAND_CLIENT_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_WAYLAND_CLIENT_LIBRARY_DIRS} ${PC_WAYLAND_CLIENT_LIBDIR}")
message(STATUS "  libs:                       ${PC_WAYLAND_CLIENT_LIBRARIES}")
message(STATUS "wayland-server:               ${WAYLAND_SERVER_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_WAYLAND_SERVER_VERSION}")
message(STATUS "  cflags:                     ${PC_WAYLAND_SERVER_CFLAGS}")
message(STATUS "  cflags other:               ${PC_WAYLAND_SERVER_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_WAYLAND_SERVER_INCLUDE_DIRS} ${PC_WAYLAND_SERVER_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_WAYLAND_SERVER_LIBRARY_DIRS} ${PC_WAYLAND_SERVER_LIBDIR}")
message(STATUS "  libs:                       ${PC_WAYLAND_SERVER_LIBRARIES}")
message(STATUS "wayland-cursor:               ${WAYLAND_CURSOR_FOUND}")
message(STATUS "  version:                    ${PC_WAYLAND_CURSOR_VERSION}")
message(STATUS "  cflags:                     ${PC_WAYLAND_CURSOR_CFLAGS}")
message(STATUS "  cflags other:               ${PC_WAYLAND_CURSOR_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_WAYLAND_CURSOR_INCLUDE_DIRS} ${PC_WAYLAND_CURSOR_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_WAYLAND_CURSOR_LIBRARY_DIRS} ${PC_WAYLAND_CURSOR_LIBDIR}")
message(STATUS "  libs:                       ${PC_WAYLAND_CURSOR_LIBRARIES}")
message(STATUS "wayland-scanner:              ${WAYLAND_SCANNER_FOUND}")
message(STATUS "  executable:                 ${PC_WAYLAND_SCANNER_wayland-scanner}")

#dump_all_vars()

# find include paths
find_path(WAYLAND_CLIENT_INCLUDE_DIR NAMES wayland-client.h HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_WAYLAND_CLIENT_INCLUDE_DIRS})
find_path(WAYLAND_SERVER_INCLUDE_DIR NAMES wayland-server.h HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_WAYLAND_SERVER_INCLUDE_DIRS})
find_path(WAYLAND_CURSOR_INCLUDE_DIR NAMES wayland-cursor.h HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_WAYLAND_CURSOR_INCLUDE_DIRS})

# find libs
find_library(WAYLAND_CLIENT_LIBRARIES NAMES ${PC_WAYLAND_CLIENT_LIBRARIES} HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_WAYLAND_CLIENT_LIBRARY_DIRS})
find_library(WAYLAND_SERVER_LIBRARIES NAMES ${PC_WAYLAND_SERVER_LIBRARIES} HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_WAYLAND_SERVER_LIBRARY_DIRS})
find_library(WAYLAND_CURSOR_LIBRARIES NAMES ${PC_WAYLAND_CURSOR_LIBRARIES} HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_WAYLAND_CURSOR_LIBRARY_DIRS})

#find executables
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner HINTS ${PC_WAYLAND_SCANNER_PREFIX}/bin)

message(STATUS "wayland-client:               ${WAYLAND_CLIENT_FOUND_TEXT}")
message(STATUS "  include dir:                ${WAYLAND_CLIENT_INCLUDE_DIR}")
message(STATUS "  libraries:                  ${WAYLAND_CLIENT_LIBRARIES}")
message(STATUS "wayland-server:               ${WAYLAND_SERVER_FOUND_TEXT}")
message(STATUS "  include dir:                ${WAYLAND_SERVER_INCLUDE_DIR}")
message(STATUS "  libraries:                  ${WAYLAND_SERVER_LIBRARIES}")
message(STATUS "wayland-cursor:               ${WAYLAND_CURSOR_FOUND}")
message(STATUS "  include dir:                ${WAYLAND_CURSOR_INCLUDE_DIR}")
message(STATUS "  libraries:                  ${WAYLAND_CURSOR_LIBRARIES}")
message(STATUS "wayland-scanner:              ${WAYLAND_SCANNER_FOUND}")
message(STATUS "  executable:                 ${WAYLAND_SCANNER_EXECUTABLE}")

# set _FOUND vars
if (WAYLAND_CLIENT_INCLUDE_DIR AND WAYLAND_CLIENT_LIBRARIES)
    set(WAYLAND_CLIENT_FOUND TRUE)
endif()
if (WAYLAND_SERVER_INCLUDE_DIR AND WAYLAND_SERVER_LIBRARIES)
    set(WAYLAND_SERVER_FOUND TRUE)
endif()
if (WAYLAND_CURSOR_INCLUDE_DIR AND WAYLAND_CURSOR_LIBRARIES)
    set(WAYLAND_CURSOR_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WAYLAND DEFAULT_MSG
        WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARIES
        WAYLAND_SERVER_INCLUDE_DIR WAYLAND_SERVER_LIBRARIES
        WAYLAND_CURSOR_INCLUDE_DIR WAYLAND_CURSOR_LIBRARIES
        WAYLAND_SCANNER_EXECUTABLE)

if (WAYLAND_CLIENT_FOUND AND WAYLAND_SERVER_FOUND AND WAYLAND_CURSOR_FOUND)
    if (WAYLAND_REQUIRED_VERSION)
        if (NOT "${WAYLAND_REQUIRED_VERSION}" STREQUAL "${PC_WAYLAND_CLIENT_VERSION}")
            message(WARNING "Incorrect version, please install wayland-${WAYLAND_REQUIRED_VERSION}")
            unset(WAYLAND_CLIENT_FOUND)
            unset(WAYLAND_SERVER_FOUND)
            unset(WAYLAND_CURSOR_FOUND)
            unset(WAYLAND_FOUND)
        endif()
    else()
        message(STATUS "Found wayland")
    endif()
else()
    message(WARNING "Could not find wayland, please install: sudo apt-get install libwayland-dev")
endif()

mark_as_advanced(
        WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_CLIENT_LIBRARIES
        WAYLAND_SERVER_INCLUDE_DIR WAYLAND_SERVER_LIBRARIES
        WAYLAND_CURSOR_INCLUDE_DIR WAYLAND_CURSOR_LIBRARIES
        WAYLAND_SCANNER_EXECUTABLE)
