# - Try to Find westeros-simpleshell
# Once done, this will define
#
#  WESTEROS_SIMPLESHELL_FOUND - system has EGL installed.
#  WESTEROS_SIMPLESHELL_INCLUDE_DIRS - directories which contain the EGL headers.
#  WESTEROS_SIMPLESHELL_LIBRARIES - libraries required to link against EGL.
#  WESTEROS_SIMPLESHELL_DEFINITIONS - Compiler switches required for using EGL.
#

set(WESTEROS_SIMPLESHELL_NAMES westeros_simpleshell_client westeros_simpleshell_server)
set(WESTEROS_SIMPLESHELL_LIBRARIES)
set(WESTEROS_SIMPLESHELL_FOUND_TEXT "Found")
foreach(LIB ${WESTEROS_SIMPLESHELL_NAMES})
    find_library(WESTEROS_SIMPLESHELL_LIBRARIES_${LIB} NAMES ${LIB}
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib
        )
    list(APPEND WESTEROS_SIMPLESHELL_LIBRARIES ${WESTEROS_SIMPLESHELL_LIBRARIES_${LIB}})
    if (NOT WESTEROS_SIMPLESHELL_LIBRARIES_${LIB})
        set(WESTEROS_SIMPLESHELL_FOUND_TEXT "Not found")
    endif()
endforeach()

message(STATUS "westeros-simpleshell-client:  ${WESTEROS_SIMPLESHELL_FOUND_TEXT}")
list_to_string(WESTEROS_SIMPLESHELL_LIBRARIES TEXT)
message(STATUS "libraries:                    ${TEXT}")

if (WESTEROS_SIMPLESHELL_LIBRARIES)
    message(STATUS "Found westeros-simpleshell")
else()
    message(SEND_ERROR "Could not find westeros-simpleshell.")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WESTEROS_SIMPLESHELL DEFAULT_MSG WESTEROS_SIMPLESHELL_LIBRARIES  )

mark_as_advanced(WESTEROS_SIMPLESHELL_LIBRARIES)