# - Try to find GLIB-2.0 libraries
#
# Copyright (C) 2017 Rene Barto
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Will be defined:
#  LIBGLIB2_FOUND - System has LibGlib2
#  LIBGLIB2_INCLUDE_DIR - The LibGlib2 include directories
#  LIBGLIB2_LIBRARIES - The libraries needed to use LibGlib2
#  LIBGLIB2_DEFINITIONS - Compiler switches required for using LibGlib2

find_package(PkgConfig)
pkg_check_modules(PC_LIBGLIB2 QUIET glib-2.0)
pkg_check_modules(PC_LIBGIO2 QUIET gio-2.0)
pkg_check_modules(PC_LIBGMODULE2 QUIET gmodule-2.0)
pkg_check_modules(PC_LIBGOBJECT2 QUIET gobject-2.0)
pkg_check_modules(PC_LIBGTHREAD2 QUIET gthread-2.0)

if (LIBGLIB2_REQUIRED_VERSION)
    set(LIBGIO2_REQUIRED_VERSION ${LIBGLIB2_REQUIRED_VERSION})
    set(LIBGMODULE2_REQUIRED_VERSION ${LIBGLIB2_REQUIRED_VERSION})
    set(LIBGOBJECT2_REQUIRED_VERSION ${LIBGLIB2_REQUIRED_VERSION})
    set(LIBGTHREAD2_REQUIRED_VERSION ${LIBGLIB2_REQUIRED_VERSION})
endif()

if(PC_LIBGLIB2_FOUND)
    set(LIBGLIB2_FOUND_TEXT "Found")
else()
    set(LIBGLIB2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGIO2_FOUND)
    set(LIBGIO2_FOUND_TEXT "Found")
else()
    set(LIBGIO2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGMODULE2_FOUND)
    set(LIBGMODULE2_FOUND_TEXT "Found")
else()
    set(LIBGMODULE2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGOBJECT2_FOUND)
    set(LIBGOBJECT2_FOUND_TEXT "Found")
else()
    set(LIBGOBJECT2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGTHREAD2_FOUND)
    set(LIBGTHREAD2_FOUND_TEXT "Found")
else()
    set(LIBGTHREAD2_FOUND_TEXT "Not found")
endif()

message(STATUS "glib-2.0:                     ${LIBGLIB2_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_LIBGLIB2_VERSION}")
message(STATUS "  cflags:                     ${PC_LIBGLIB2_CFLAGS}")
message(STATUS "  cflags other:               ${PC_LIBGLIB2_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_LIBGLIB2_INCLUDE_DIRS} ${PC_LIBGLIB2_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_LIBGLIB2_LIBRARY_DIRS} ${PC_LIBGLIB2_LIBDIR}")
message(STATUS "  ldflags:                    ${PC_LIBGLIB2_LDFLAGS}")
message(STATUS "  ldflags other:              ${PC_LIBGLIB2_LDFLAGS_OTHER}")
message(STATUS "  libs:                       ${PC_LIBGLIB2_LIBRARIES}")
message(STATUS "gio-2.0:                      ${LIBGIO2_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_LIBGIO2_VERSION}")
message(STATUS "  cflags:                     ${PC_LIBGIO2_CFLAGS}")
message(STATUS "  cflags other:               ${PC_LIBGIO2_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_LIBGIO2_INCLUDE_DIRS} ${PC_LIBGIO2_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_LIBGIO2_LIBRARY_DIRS} ${PC_LIBGIO2_LIBDIR}")
message(STATUS "  ldflags:                    ${PC_LIBGIO2_LDFLAGS}")
message(STATUS "  ldflags other:              ${PC_LIBGIO2_LDFLAGS_OTHER}")
message(STATUS "  libs:                       ${PC_LIBGIO2_LIBRARIES}")
message(STATUS "gmodule-2.0:                  ${LIBGMODULE2_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_LIBGMODULE2_VERSION}")
message(STATUS "  cflags:                     ${PC_LIBGMODULE2_CFLAGS}")
message(STATUS "  cflags other:               ${PC_LIBGMODULE2_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_LIBGMODULE2_INCLUDE_DIRS} ${PC_LIBGMODULE2_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_LIBGMODULE2_LIBRARY_DIRS} ${PC_LIBGMODULE2_LIBDIR}")
message(STATUS "  ldflags:                    ${PC_LIBGMODULE2_LDFLAGS}")
message(STATUS "  ldflags other:              ${PC_LIBGMODULE2_LDFLAGS_OTHER}")
message(STATUS "  libs:                       ${PC_LIBGMODULE2_LIBRARIES}")
message(STATUS "gobject-2.0:                  ${LIBGOBJECT2_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_LIBGOBJECT2_VERSION}")
message(STATUS "  cflags:                     ${PC_LIBGOBJECT2_CFLAGS}")
message(STATUS "  cflags other:               ${PC_LIBGOBJECT2_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_LIBGOBJECT2_INCLUDE_DIRS} ${PC_LIBGOBJECT2_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_LIBGOBJECT2_LIBRARY_DIRS} ${PC_LIBGOBJECT2_LIBDIR}")
message(STATUS "  ldflags:                    ${PC_LIBGOBJECT2_LDFLAGS}")
message(STATUS "  ldflags other:              ${PC_LIBGOBJECT2_LDFLAGS_OTHER}")
message(STATUS "  libs:                       ${PC_LIBGOBJECT2_LIBRARIES}")
message(STATUS "gthread-2.0:                  ${LIBLIBGTHREAD2_FOUND_TEXT}")
message(STATUS "  version:                    ${PC_LIBGTHREAD2_VERSION}")
message(STATUS "  cflags:                     ${PC_LIBGTHREAD2_CFLAGS}")
message(STATUS "  cflags other:               ${PC_LIBGTHREAD2_CFLAGS_OTHER}")
message(STATUS "  include dirs:               ${PC_LIBGTHREAD2_INCLUDE_DIRS} ${PC_LIBGTHREAD2_INCLUDEDIR}")
message(STATUS "  lib dirs:                   ${PC_LIBGTHREAD2_LIBRARY_DIRS} ${PC_LIBGTHREAD2_LIBDIR}")
message(STATUS "  ldflags:                    ${PC_LIBGTHREAD2_LDFLAGS}")
message(STATUS "  ldflags other:              ${PC_LIBGTHREAD2_LDFLAGS_OTHER}")
message(STATUS "  libs:                       ${PC_LIBGTHREAD2_LIBRARIES}")

if(PC_LIBGLIB2_FOUND)
    if(LIBGLIB2_REQUIRED_VERSION)
        if (NOT "${LIBGLIB2_REQUIRED_VERSION}" STREQUAL "${PC_LIBGLIB2_VERSION}")
            message(WARNING "Incorrect version, please install libglib2-${LIBGLIB2_REQUIRED_VERSION}")
            set(LIBGLIB2_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBGLIB2_FOUND)
        endif()
    endif()
else()
    set(LIBGLIB2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGIO2_FOUND)
    if(LIBGIO2_REQUIRED_VERSION)
        if (NOT "${LIBGIO2_REQUIRED_VERSION}" STREQUAL "${PC_LIBGIO2_VERSION}")
            message(WARNING "Incorrect version, please install libgio2-${LIBGIO2_REQUIRED_VERSION}")
            set(LIBGIO2_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBGIO2_FOUND)
        endif()
    endif()
else()
    set(LIBGIO2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGMODULE2_FOUND)
    if(LIBGMODULE2_REQUIRED_VERSION)
        if (NOT "${LIBGMODULE2_REQUIRED_VERSION}" STREQUAL "${PC_LIBGMODULE2_VERSION}")
            message(WARNING "Incorrect version, please install libgmodule2-${LIBGMODULE2_REQUIRED_VERSION}")
            set(LIBGMODULE2_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBGMODULE2_FOUND)
        endif()
    endif()
else()
    set(LIBGMODULE2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGOBJECT2_FOUND)
    if(LIBGOBJECT2_REQUIRED_VERSION)
        if (NOT "${LIBGOBJECT2_REQUIRED_VERSION}" STREQUAL "${PC_LIBGOBJECT2_VERSION}")
            message(WARNING "Incorrect version, please install libgobject2-${LIBGOBJECT2_REQUIRED_VERSION}")
            set(LIBGOBJECT2_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBGOBJECT2_FOUND)
        endif()
    endif()
else()
    set(LIBGOBJECT2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGTHREAD2_FOUND)
    if(LIBGTHREAD2_REQUIRED_VERSION)
        if (NOT "${LIBGTHREAD2_REQUIRED_VERSION}" STREQUAL "${PC_LIBGTHREAD2_VERSION}")
            message(WARNING "Incorrect version, please install libgthread2-${LIBGTHREAD2_REQUIRED_VERSION}")
            set(LIBGTHREAD2_FOUND_TEXT "Found incorrect version")
            unset(PC_LIBGTHREAD2_FOUND)
        endif()
    endif()
else()
    set(LIBGTHREAD2_FOUND_TEXT "Not found")
endif()

if(PC_LIBGLIB2_FOUND)
    find_path(LIBGLIB2_CONFIG_INCLUDE_DIR NAMES glibconfig.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib/glib-2.0/include ${PC_LIBGLIB2_INCLUDEDIR} ${PC_LIBGLIB2_INCLUDE_DIRS})

    find_path(LIBGLIB2_INCLUDE_DIR NAMES glib.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include/glib-2.0 ${PC_LIBGLIB2_INCLUDEDIR} ${PC_LIBGLIB2_INCLUDE_DIRS})

    find_library(LIBGLIB2_LIBRARY NAMES glib-2.0
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIBGLIB2_LIBDIR} ${PC_LIBGLIB2_LIBRARY_DIRS})

    if(NOT LIBGLIB2_INCLUDE_DIR OR NOT LIBGLIB2_LIBRARY)
        set(LIBGLIB2_FOUND_TEXT "Not found")
    else()
        set(LIBGLIB2_FOUND_TEXT "Found")
    endif()
endif()

if(PC_LIBGIO2_FOUND)
    find_path(LIBGIO2_INCLUDE_DIR NAMES gio/gio.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include/glib-2.0 ${PC_LIBGIO2_INCLUDEDIR} ${PC_LIBGIO2_INCLUDE_DIRS})

    find_library(LIBGIO2_LIBRARY NAMES gio-2.0
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIBGIO2_LIBDIR} ${PC_LIBGIO2_LIBRARY_DIRS})

    if(NOT LIBGIO2_INCLUDE_DIR OR NOT LIBGIO2_LIBRARY)
        set(LIBGIO2_FOUND_TEXT "Not found")
    else()
        set(LIBGIO2_FOUND_TEXT "Found")
    endif()
endif()

if(PC_LIBGMODULE2_FOUND)
    find_path(LIBGMODULE2_INCLUDE_DIR NAMES gmodule.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include/glib-2.0 ${PC_LIBGMODULE2_INCLUDEDIR} ${PC_LIBGMODULE2_INCLUDE_DIRS})

    find_library(LIBGMODULE2_LIBRARY NAMES gmodule-2.0
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIBGMODULE2_LIBDIR} ${PC_LIBGMODULE2_LIBRARY_DIRS})

    if(NOT LIBGMODULE2_INCLUDE_DIR OR NOT LIBGMODULE2_LIBRARY)
        set(LIBGMODULE2_FOUND_TEXT "Not found")
    else()
        set(LIBGMODULE2_FOUND_TEXT "Found")
    endif()
endif()

if(PC_LIBGOBJECT2_FOUND)
    find_path(LIBGOBJECT2_INCLUDE_DIR NAMES gobject/gobject.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include/glib-2.0 ${PC_LIBGOBJECT2_INCLUDEDIR} ${PC_LIBGOBJECT2_INCLUDE_DIRS})

    find_library(LIBGOBJECT2_LIBRARY NAMES gobject-2.0
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIBGOBJECT2_LIBDIR} ${PC_LIBGOBJECT2_LIBRARY_DIRS})

    if(NOT LIBGOBJECT2_INCLUDE_DIR OR NOT LIBGOBJECT2_LIBRARY)
        set(LIBGOBJECT2_FOUND_TEXT "Not found")
    else()
        set(LIBGOBJECT2_FOUND_TEXT "Found")
    endif()
endif()

if(PC_LIBGTHREAD2_FOUND)
    find_path(LIBGTHREAD2_INCLUDE_DIR NAMES glib/gthread.h glib/gthreadpool.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include/glib-2.0 ${PC_LIBGTHREAD2_INCLUDEDIR} ${PC_LIBGTHREAD2_INCLUDE_DIRS})

    find_library(LIBGTHREAD2_LIBRARY NAMES gthread-2.0
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIBGTHREAD2_LIBDIR} ${PC_LIBGTHREAD2_LIBRARY_DIRS})

    if(NOT LIBGTHREAD2_INCLUDE_DIR OR NOT LIBGTHREAD2_LIBRARY)
        set(LIBGTHREAD2_FOUND_TEXT "Not found")
    else()
        set(LIBGTHREAD2_FOUND_TEXT "Found")
    endif()
endif()

set(LIBGLIB2_DEFINITIONS ${PC_LIBGLIB2_CFLAGS_OTHER})
set(LIBGIO2_DEFINITIONS ${PC_LIBGIO2_CFLAGS_OTHER})
set(LIBGMODULE2_DEFINITIONS ${PC_LIBGMODULE2_CFLAGS_OTHER})
set(LIBGOBJECT2_DEFINITIONS ${PC_LIBGOBJECT2_CFLAGS_OTHER})
set(LIBGTHREAD2_DEFINITIONS ${PC_LIBGTHREAD2_CFLAGS_OTHER})

message(STATUS "libglib2-config:              ")
message(STATUS "  include dirs:               ${LIBGLIB2_CONFIG_INCLUDE_DIR}")
message(STATUS "libglib-2.0:                  ${LIBGLIB2_FOUND_TEXT}")
message(STATUS "  include dirs:               ${LIBGLIB2_INCLUDE_DIR}")
message(STATUS "  libs:                       ${LIBGLIB2_LIBRARY}")
message(STATUS "libgio-2.0:                   ${LIBGIO2_FOUND_TEXT}")
message(STATUS "  include dirs:               ${LIBGIO2_INCLUDE_DIR}")
message(STATUS "  libs:                       ${LIBGIO2_LIBRARY}")
message(STATUS "libgmodule-2.0:               ${LIBGMODULE2_FOUND_TEXT}")
message(STATUS "  include dirs:               ${LIBGMODULE2_INCLUDE_DIR}")
message(STATUS "  libs:                       ${LIBGMODULE2_LIBRARY}")
message(STATUS "libgobjects-2.0:              ${LIBGOBJECT2_FOUND_TEXT}")
message(STATUS "  include dirs:               ${LIBGOBJECT2_INCLUDE_DIR}")
message(STATUS "  libs:                       ${LIBGOBJECT2_LIBRARY}")
message(STATUS "libgthread-2.0:               ${LIBGTHREAD2_FOUND_TEXT}")
message(STATUS "  include dirs:               ${LIBGTHREAD2_INCLUDE_DIR}")
message(STATUS "  libs:                       ${LIBGTHREAD2_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBGLIB2 DEFAULT_MSG
    LIBGLIB2_LIBRARY LIBGLIB2_INCLUDE_DIR LIBGLIB2_CONFIG_INCLUDE_DIR)
find_package_handle_standard_args(LIBGIO2 DEFAULT_MSG
    LIBGIO2_LIBRARY LIBGIO2_INCLUDE_DIR)
find_package_handle_standard_args(LIBGMODULE2 DEFAULT_MSG
    LIBGMODULE2_LIBRARY LIBGMODULE2_INCLUDE_DIR)
find_package_handle_standard_args(LIBGOBJECT2 DEFAULT_MSG
    LIBGOBJECT2_LIBRARY LIBGOBJECT2_INCLUDE_DIR)
find_package_handle_standard_args(LIBGTHREAD2 DEFAULT_MSG
    LIBGTHREAD2_LIBRARY LIBGTHREAD2_INCLUDE_DIR)

if(LIBGLIB2_FOUND)
else()
    message(WARNING "Could not find libglib2, please install: sudo apt-get install libglib2.0-dev")
endif()
if(LIBGIO2_FOUND)
else()
    message(WARNING "Could not find libgio2, please install: sudo apt-get install libglib2.0-dev")
endif()
if(LIBGMODULE2_FOUND)
else()
    message(WARNING "Could not find libgmodule2, please install: sudo apt-get install libglib2.0-dev")
endif()
if(LIBGOBJECT2_FOUND)
else()
    message(WARNING "Could not find libgobject2, please install: sudo apt-get install libglib2.0-dev")
endif()
if(LIBGTHREAD2_FOUND)
else()
    message(WARNING "Could not find libgthread2, please install: sudo apt-get install libglib2.0-dev")
endif()

mark_as_advanced(LIBGLIB2_DEFINITIONS
    LIBGLIB2_CONFIG_INCLUDE_DIR
    LIBGLIB2_INCLUDE_DIR LIBGLIB2_LIBRARY
    LIBGIO2_INCLUDE_DIR LIBGIO2_LIBRARY
    LIBGMODULE2_INCLUDE_DIR LIBGMODULE2_LIBRARY
    LIBGOBJECT2_INCLUDE_DIR LIBGOBJECT2_LIBRARY
    LIBGTHREAD2_INCLUDE_DIR LIBGTHREAD2_LIBRARY)

