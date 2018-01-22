# - Try to Find libffi
# Once done, this will define
#
#  LIB_FFI_FOUND - system has EGL installed.
#  LIB_FFI_INCLUDE_DIRS - directories which contain the EGL headers.
#  LIB_FFI_LIBRARIES - libraries required to link against EGL.
#  LIB_FFI_DEFINITIONS - Compiler switches required for using EGL.
#


find_package(PkgConfig)

pkg_check_modules(PC_LIB_FFI libffi)

if (PC_LIB_FFI_FOUND)
    message(STATUS "Found libffi.pc")
    message(STATUS "PC_LIB_FFI_CFLAGS:            ${PC_LIB_FFI_CFLAGS}")
    message(STATUS "PC_LIB_FFI_CFLAGS_OTHER:      ${PC_LIB_FFI_CFLAGS_OTHER}")
    message(STATUS "PC_LIB_FFI_INCLUDEDIR:        ${PC_LIB_FFI_INCLUDEDIR}")
    message(STATUS "PC_LIB_FFI_INCLUDE_DIRS:      ${PC_LIB_FFI_INCLUDE_DIRS}")
    message(STATUS "PC_LIB_FFI_LIBDIR:            ${PC_LIB_FFI_LIBDIR}")
    message(STATUS "PC_LIB_FFI_LIBRARY_DIRS:      ${PC_LIB_FFI_LIBRARY_DIRS}")
    message(STATUS "PC_LIB_FFI_LDFLAGS:           ${PC_LIB_FFI_LDFLAGS}")
    message(STATUS "PC_LIB_FFI_LDFLAGS_OTHER:     ${PC_LIB_FFI_LDFLAGS_OTHER}")
    message(STATUS "PC_LIB_FFI_LIBRARIES:         ${PC_LIB_FFI_LIBRARIES}")

    set(LIB_FFI_DEFINITIONS ${PC_LIB_FFI_CFLAGS_OTHER})
endif ()

find_path(LIB_FFI_INCLUDE_DIRECTORY NAMES ffi.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_LIB_FFI_INCLUDEDIR} ${PC_LIB_FFI_INCLUDE_DIRS}
        )

set(LIB_FFI_NAMES ${PC_LIB_FFI_LIBRARIES})
find_library(LIB_FFI_LIBRARIES NAMES ${LIB_FFI_NAMES}
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_LIB_FFI_LIBDIR} ${PC_LIB_FFI_LIBRARY_DIRS}
        )
message(STATUS "LIB_FFI_INCLUDE_DIRECTORY:    ${LIB_FFI_INCLUDE_DIRECTORY}")
message(STATUS "LIB_FFI_LIBRARIES:            ${LIB_FFI_LIBRARIES}")

if (LIB_FFI_INCLUDE_DIRECTORY AND LIB_FFI_LIBRARIES)
    message(STATUS "Found libffi")
else()
    message(SEND_ERROR "Could not find libffi. Please install: sudo apt-get install libffi-dev")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIB_FFI DEFAULT_MSG LIB_FFI_INCLUDE_DIRECTORY LIB_FFI_LIBRARIES)

mark_as_advanced(LIB_FFI_INCLUDE_DIRECTORY LIB_FFI_LIBRARIES)