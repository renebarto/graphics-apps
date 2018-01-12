# - Try to Find gbm
# Once done, this will define
#
#  GBM_FOUND - system has EGL installed.
#  GBM_INCLUDE_DIRS - directories which contain the EGL headers.
#  GBM_LIBRARIES - libraries required to link against EGL.
#  GBM_DEFINITIONS - Compiler switches required for using EGL.
#


find_package(PkgConfig)

pkg_check_modules(PC_GBM gbm)

if (PC_GBM_FOUND)
    message(STATUS "Found libgbm.pc")
    message(STATUS "PC_GBM_CFLAGS:           ${PC_GBM_CFLAGS}")
    message(STATUS "PC_GBM_CFLAGS_OTHER:     ${PC_GBM_CFLAGS_OTHER}")
    message(STATUS "PC_GBM_INCLUDEDIR:       ${PC_GBM_INCLUDEDIR}")
    message(STATUS "PC_GBM_INCLUDE_DIRS:     ${PC_GBM_INCLUDE_DIRS}")
    message(STATUS "PC_GBM_LIBDIR:           ${PC_GBM_LIBDIR}")
    message(STATUS "PC_GBM_LIBRARY_DIRS:     ${PC_GBM_LIBRARY_DIRS}")
    message(STATUS "PC_GBM_LDFLAGS:          ${PC_GBM_LDFLAGS}")
    message(STATUS "PC_GBM_LDFLAGS_OTHER:    ${PC_GBM_LDFLAGS_OTHER}")
    message(STATUS "PC_GBM_LIBRARIES:        ${PC_GBM_LIBRARIES}")

    set(GBM_DEFINITIONS ${PC_GBM_CFLAGS_OTHER})
endif ()

find_path(GBM_INCLUDE_DIRECTORY NAMES gbm.h
        HINTS ${PC_GBM_INCLUDEDIR} ${PC_GBM_INCLUDE_DIRS}
        )

set(GBM_NAMES ${PC_GBM_LIBRARIES})
find_library(GBM_LIBRARIES NAMES ${GBM_NAMES}
        HINTS ${PC_GBM_LIBDIR} ${PC_GBM_LIBRARY_DIRS}
        )

if (NOT ${GBM_INCLUDE_DIRECTORY} STREQUAL "" AND
    NOT ${GBM_LIBRARIES} STREQUAL "")
    message(STATUS "Found libgbm")
else()
    message(SEND_ERROR "Could not find libgbm. Please install: sudo apt-get install libgbm-dev")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GBM DEFAULT_MSG GBM_INCLUDE_DIRECTORY GBM_LIBRARIES)

mark_as_advanced(GBM_INCLUDE_DIRECTORY GBM_LIBRARIES)