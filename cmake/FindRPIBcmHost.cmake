# - Try to Find RPI BCM host
# Once done, this will define
#
#  RPI_BCM_HOST_FOUND - system has EGL installed.
#  RPI_BCM_HOST_INCLUDE_DIRS - directories which contain the EGL headers.
#  RPI_BCM_HOST_LIBRARIES - libraries required to link against EGL.
#  RPI_BCM_HOST_DEFINITIONS - Compiler switches required for using EGL.
#


find_package(PkgConfig)

pkg_check_modules(PC_RPI_BCM_HOST bcm_host)

if (PC_RPI_BCM_HOST_FOUND)
    message(STATUS "Found bcm_host.pc")
    message(STATUS "PC_RPI_BCM_HOST_CFLAGS:           ${PC_RPI_BCM_HOST_CFLAGS}")
    message(STATUS "PC_RPI_BCM_HOST_CFLAGS_OTHER:     ${PC_RPI_BCM_HOST_CFLAGS_OTHER}")
    message(STATUS "PC_RPI_BCM_HOST_INCLUDEDIR:       ${PC_RPI_BCM_HOST_INCLUDEDIR}")
    message(STATUS "PC_RPI_BCM_HOST_INCLUDE_DIRS:     ${PC_RPI_BCM_HOST_INCLUDE_DIRS}")
    message(STATUS "PC_RPI_BCM_HOST_LIBDIR:           ${PC_RPI_BCM_HOST_LIBDIR}")
    message(STATUS "PC_RPI_BCM_HOST_LIBRARY_DIRS:     ${PC_RPI_BCM_HOST_LIBRARY_DIRS}")
    message(STATUS "PC_RPI_BCM_HOST_LDFLAGS:          ${PC_RPI_BCM_HOST_LDFLAGS}")
    message(STATUS "PC_RPI_BCM_HOST_LDFLAGS_OTHER:    ${PC_RPI_BCM_HOST_LDFLAGS_OTHER}")
    message(STATUS "PC_RPI_BCM_HOST_LIBRARIES:        ${PC_RPI_BCM_HOST_LIBRARIES}")

    set(RPI_BCM_HOST_DEFINITIONS ${PC_RPI_BCM_HOST_CFLAGS_OTHER})
endif ()

if (PC_RPI_BCM_HOST_FOUND)
    find_path(RPI_BCM_HOST_INCLUDE_DIRECTORY NAMES bcm_host.h
            HINTS ${CMAKE_FRAMEWORK_PATH}/include ${PC_RPI_BCM_HOST_INCLUDEDIR} ${PC_RPI_BCM_HOST_INCLUDE_DIRS}
            )
    
    set(RPI_BCM_HOST_FOUND_TEXT "Found")
    set(RPI_BCM_HOST_LIBRARIES )
    foreach(LIB ${PC_RPI_BCM_HOST_LIBRARIES})
        find_library(RPI_BCM_HOST_LIBRARIES_${LIB} NAMES ${LIB}
            HINTS ${CMAKE_FRAMEWORK_PATH}/lib ${PC_RPI_BCM_HOST_LIBDIR} ${PC_RPI_BCM_HOST_LIBRARY_DIRS}
            )
        list(APPEND RPI_BCM_HOST_LIBRARIES ${RPI_BCM_HOST_LIBRARIES_${LIB}})
        if (NOT RPI_BCM_HOST_LIBRARIES_${LIB})
            set(RPI_BCM_HOST_FOUND_TEXT "Not found")
        endif()
    endforeach()

    if(NOT RPI_BCM_HOST_INCLUDE_DIRECTORY OR NOT RPI_BCM_HOST_LIBRARIES)
        set(RPI_BCM_HOST_FOUND_TEXT "Not found")
    else()
        set(RPI_BCM_HOST_FOUND_TEXT "Found")
    endif()
else()
    set(RPI_BCM_HOST_FOUND_TEXT "Found")
endif()

message(STATUS "bcm-host:                       ${RPI_BCM_HOST_FOUND_TEXT}")
list_to_string(RPI_BCM_HOST_LIBRARIES TEXT)
message(STATUS "RPI_BCM_HOST_INCLUDE_DIRECTORY: ${RPI_BCM_HOST_INCLUDE_DIRECTORY}")
message(STATUS "RPI_BCM_HOST_LIBRARIES:         ${TEXT}")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RPI_BCM_HOST DEFAULT_MSG RPI_BCM_HOST_INCLUDE_DIRECTORY RPI_BCM_HOST_LIBRARIES)

if (RPI_BCM_HOST_FOUND)
    message(STATUS "Found bcm_host")
else()
    message(SEND_ERROR "Could not find bcm_host.")
endif()


mark_as_advanced(RPI_BCM_HOST_INCLUDE_DIRECTORY RPI_BCM_HOST_LIBRARIES)