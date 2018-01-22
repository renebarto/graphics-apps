# - Try to Find RPI VC host interface
# Once done, this will define
#
#  VC_HOST_IF_FOUND - system has EGL installed.
#  VC_HOST_IF_INCLUDE_DIRS - directories which contain the EGL headers.
#  VC_HOST_IF_LIBRARIES - libraries required to link against EGL.
#  VC_HOST_IF_DEFINITIONS - Compiler switches required for using EGL.
#


find_path(VC_HOST_IF_INCLUDE_DIRECTORY NAMES interface/vmcs_host/vchost.h
        HINTS ${CMAKE_FRAMEWORK_PATH}/include
        )

find_library(VC_HOST_IF_LIBRARY NAMES vchostif
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )

if(NOT VC_HOST_IF_INCLUDE_DIRECTORY OR NOT VC_HOST_IF_LIBRARY)
    set(VC_HOST_IF_FOUND_TEXT "Not found")
else()
    set(VC_HOST_IF_FOUND_TEXT "Found")
endif()

message(STATUS "vchostif:                       ${VC_HOST_IF_FOUND_TEXT}")
message(STATUS "VC_HOST_IF_INCLUDE_DIRECTORY:   ${VC_HOST_IF_INCLUDE_DIRECTORY}")
message(STATUS "VC_HOST_IF_LIBRARY:             ${VC_HOST_IF_LIBRARY}")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VC_HOST_IF DEFAULT_MSG VC_HOST_IF_INCLUDE_DIRECTORY VC_HOST_IF_LIBRARY)

if (VC_HOST_IF_FOUND)
    message(STATUS "Found vchostif")
else()
    message(SEND_ERROR "Could not find vchostif.")
endif()


mark_as_advanced(VC_HOST_IF_INCLUDE_DIRECTORY VC_HOST_IF_LIBRARY)