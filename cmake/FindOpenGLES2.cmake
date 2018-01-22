#-------------------------------------------------------------------
# This file is stolen from part of the CMake build system for OGRE (Object-oriented Graphics Rendering Engine) http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenGLES
# Once done this will define
#
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLESv2
#  OPENGLES1_gl_LIBRARY   - Link these to use OpenGLESv1
#
# Win32, Apple, and Android are not tested!
# Linux tested and works

find_package(PkgConfig)

pkg_check_modules(PC_OPENGLES2 glesv2)

if (PC_OPENGLES2_FOUND)
    message(STATUS "Found glesv2.pc")
    message(STATUS "PC_OPENGLES2_CFLAGS:          ${PC_OPENGLES2_CFLAGS}")
    message(STATUS "PC_OPENGLES2_CFLAGS_OTHER:    ${PC_OPENGLES2_CFLAGS_OTHER}")
    message(STATUS "PC_OPENGLES2_INCLUDEDIR:      ${PC_OPENGLES2_INCLUDEDIR}")
    message(STATUS "PC_OPENGLES2_INCLUDE_DIRS:    ${PC_OPENGLES2_INCLUDE_DIRS}")
    message(STATUS "PC_OPENGLES2_LIBDIR:          ${PC_OPENGLES2_LIBDIR}")
    message(STATUS "PC_OPENGLES2_LIBRARY_DIRS:    ${PC_OPENGLES2_LIBRARY_DIRS}")
    message(STATUS "PC_OPENGLES2_LDFLAGS:         ${PC_OPENGLES2_LDFLAGS}")
    message(STATUS "PC_OPENGLES2_LDFLAGS_OTHER:   ${PC_OPENGLES2_LDFLAGS_OTHER}")

    set(OPENGLES2_DEFINITIONS ${PC_OPENGLES2_CFLAGS_OTHER})
endif ()

find_path(OPENGLES2_INCLUDE_DIRECTORY NAMES GLES2/gl2.h
    HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_OPENGLES2_INCLUDEDIR} ${PC_OPENGLES2_INCLUDE_DIRS}
    )

set(OPENGLES2_NAMES GLESv2)
find_library(OPENGLES2_LIBRARIES NAMES ${OPENGLES2_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_OPENGLES2_LIBDIR} ${PC_OPENGLES2_LIBRARY_DIRS}
    )

message(STATUS "OPENGLES2_INCLUDE_DIRECTORY:  ${OPENGLES2_INCLUDE_DIRECTORY}")
message(STATUS "OPENGLES2_LIBRARIES:          ${OPENGLES2_LIBRARIES}")

if (NOT ${OPENGLES2_INCLUDE_DIRECTORY} STREQUAL "" AND
    NOT ${OPENGLES2_LIBRARIES} STREQUAL "")
    message(STATUS "Found GLESv2")
else()
    message(SEND_ERROR "Could not find GLESv2.")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EGL DEFAULT_MSG OPENGLES2_INCLUDE_DIRECTORY OPENGLES2_LIBRARIES)

mark_as_advanced(OPENGLES2_INCLUDE_DIRECTORY OPENGLES2_LIBRARIES)
