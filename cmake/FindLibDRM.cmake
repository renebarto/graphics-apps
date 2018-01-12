# - Try to Find libdrm
# Once done, this will define
#
#  LIB_DRM_FOUND - system has EGL installed.
#  LIB_DRM_INCLUDE_DIRS - directories which contain the EGL headers.
#  LIB_DRM_LIBRARIES - libraries required to link against EGL.
#  LIB_DRM_DEFINITIONS - Compiler switches required for using EGL.
#


find_package(PkgConfig)

pkg_check_modules(PC_LIB_DRM libdrm)

if (PC_LIB_DRM_FOUND)
    message(STATUS "Found libdrm.pc")
    message(STATUS "PC_LIB_DRM_CFLAGS:           ${PC_LIB_DRM_CFLAGS}")
    message(STATUS "PC_LIB_DRM_CFLAGS_OTHER:     ${PC_LIB_DRM_CFLAGS_OTHER}")
    message(STATUS "PC_LIB_DRM_INCLUDEDIR:       ${PC_LIB_DRM_INCLUDEDIR}")
    message(STATUS "PC_LIB_DRM_INCLUDE_DIRS:     ${PC_LIB_DRM_INCLUDE_DIRS}")
    message(STATUS "PC_LIB_DRM_LIBDIR:           ${PC_LIB_DRM_LIBDIR}")
    message(STATUS "PC_LIB_DRM_LIBRARY_DIRS:     ${PC_LIB_DRM_LIBRARY_DIRS}")
    message(STATUS "PC_LIB_DRM_LDFLAGS:          ${PC_LIB_DRM_LDFLAGS}")
    message(STATUS "PC_LIB_DRM_LDFLAGS_OTHER:    ${PC_LIB_DRM_LDFLAGS_OTHER}")
    message(STATUS "PC_LIB_DRM_LIBRARIES:        ${PC_LIB_DRM_LIBRARIES}")

    set(LIB_DRM_DEFINITIONS ${PC_LIB_DRM_CFLAGS_OTHER})
endif ()

find_path(LIB_DRM_INCLUDE_DIRECTORY NAMES drm.h
        HINTS ${PC_LIB_DRM_INCLUDEDIR} ${PC_LIB_DRM_INCLUDE_DIRS}
        )

set(LIB_DRM_NAMES ${PC_LIB_DRM_LIBRARIES})
find_library(LIB_DRM_LIBRARIES NAMES ${LIB_DRM_NAMES}
        HINTS ${PC_LIB_DRM_LIBDIR} ${PC_LIB_DRM_LIBRARY_DIRS}
        )

if (NOT ${LIB_DRM_INCLUDE_DIRECTORY} STREQUAL "" AND
    NOT ${LIB_DRM_LIBRARIES} STREQUAL "")
    message(STATUS "Found libdrm")
else()
    message(SEND_ERROR "Could not find libdrm. Please install: sudo apt-get install libdrm-dev")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIB_DRM DEFAULT_MSG LIB_DRM_INCLUDE_DIRECTORY LIB_DRM_LIBRARIES)

mark_as_advanced(LIB_DRM_INCLUDE_DIRECTORY LIB_DRM_LIBRARIES)