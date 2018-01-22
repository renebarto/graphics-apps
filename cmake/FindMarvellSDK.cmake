# - Try to Find Marvell SDK
# Once done, this will define
#
#  MARVELL_AMP_CLIENT_FOUND - system has EGL installed.
#  MARVELL_AMP_CLIENT_INCLUDE_DIRS - directories which contain the EGL headers.
#  MARVELL_AMP_CLIENT_LIBRARIES - libraries required to link against EGL.
#  MARVELL_OSAL_FOUND - system has EGL installed.
#  MARVELL_OSAL_INCLUDE_DIRS - directories which contain the EGL headers.
#  MARVELL_OSAL_LIBRARIES - libraries required to link against EGL.
#  MARVELL_GFX_FOUND - system has EGL installed.
#  MARVELL_GFX_INCLUDE_DIRS - directories which contain the EGL headers.
#  MARVELL_GFX_LIBRARIES - libraries required to link against EGL.
#  MARVELL_LOG_FOUND - system has EGL installed.
#  MARVELL_LOG_INCLUDE_DIRS - directories which contain the EGL headers.
#  MARVELL_LOG_LIBRARIES - libraries required to link against EGL.
#

find_path(MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY NAMES amp_client.h
    HINTS ${CMAKE_FRAMEWORK_PATH}/include/marvell/amp/inc
    )
set(MARVELL_AMP_CLIENT_NAMES ampclient)
find_library(MARVELL_AMP_CLIENT_LIBRARIES NAMES ${MARVELL_AMP_CLIENT_NAMES}
        HINTS ${CMAKE_FRAMEWORK_PATH}/lib
        )

find_path(MARVELL_OSAL_INCLUDE_DIRECTORY NAMES OSAL_api.h
    HINTS ${CMAKE_FRAMEWORK_PATH}/include/marvell/osal/include
    )
set(MARVELL_OSAL_NAMES OSAL)
find_library(MARVELL_OSAL_LIBRARIES NAMES ${MARVELL_OSAL_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )

set(MARVELL_GFX_NAMES graphics)
find_library(MARVELL_GFX_LIBRARIES NAMES ${MARVELL_GFX_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )

set(MARVELL_LOG_NAMES log)
find_library(MARVELL_LOG_LIBRARIES NAMES ${MARVELL_LOG_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )

find_path(MARVELL_LIBAV_INCLUDE_DIRECTORY NAMES libavcodec/avcodec.h libavformat/avformat.h libavutil/avutil.h
    HINTS ${CMAKE_FRAMEWORK_PATH}/include/
    )
set(MARVELL_LIBAV_NAMES avcodec)
find_library(MARVELL_LIBAV_LIBRARIES_AVCODEC NAMES ${MARVELL_LIBAV_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )
set(MARVELL_LIBAV_NAMES avformat)
find_library(MARVELL_LIBAV_LIBRARIES_AVFORMAT NAMES ${MARVELL_LIBAV_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )
set(MARVELL_LIBAV_NAMES avutil)
find_library(MARVELL_LIBAV_LIBRARIES_AVUTIL NAMES ${MARVELL_LIBAV_NAMES}
    HINTS ${CMAKE_FRAMEWORK_PATH}/lib
    )

if (MARVELL_AMP_CLIENT_LIBRARIES AND MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY)
    set(MARVELL_AMP_CLIENT_FOUND "Found")
else()
    set(MARVELL_AMP_CLIENT_FOUND "Not found")
endif()
if (MARVELL_OSAL_LIBRARIES AND MARVELL_OSAL_INCLUDE_DIRECTORY)
    set(MARVELL_OSAL_FOUND "Found")
else()
    set(MARVELL_OSAL_FOUND "Not found")
endif()
if (MARVELL_GFX_LIBRARIES)
    set(MARVELL_GFX_FOUND "Found")
else()
    set(MARVELL_GFX_FOUND "Not found")
endif()
if (MARVELL_LOG_LIBRARIES)
    set(MARVELL_LOG_FOUND "Found")
else()
    set(MARVELL_LOG_FOUND "Not found")
endif()
if (MARVELL_LIBAV_LIBRARIES_AVCODEC AND MARVELL_LIBAV_LIBRARIES_AVFORMAT AND MARVELL_LIBAV_LIBRARIES_AVUTIL)
    set(MARVELL_LIBAV_FOUND "Found")
else()
    set(MARVELL_LIBAV_FOUND "Not found")
endif()
message(STATUS "ampclient:                    ${MARVELL_AMP_CLIENT_FOUND}")
message(STATUS "includes:                     ${MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY}")
message(STATUS "library:                      ${MARVELL_AMP_CLIENT_LIBRARIES}")
message(STATUS "osal:                         ${MARVELL_OSAL_FOUND}")
message(STATUS "includes:                     ${MARVELL_OSAL_INCLUDE_DIRECTORY}")
message(STATUS "library:                      ${MARVELL_OSAL_LIBRARIES}")
message(STATUS "graphics:                     ${MARVELL_GFX_FOUND}")
message(STATUS "library:                      ${MARVELL_GFX_LIBRARIES}")
message(STATUS "log:                          ${MARVELL_LOG_FOUND}")
message(STATUS "library:                      ${MARVELL_LOG_LIBRARIES}")
message(STATUS "libav:                        ${MARVELL_LIBAV_FOUND}")
message(STATUS "includes:                     ${MARVELL_LIBAV_INCLUDE_DIRECTORY}")
message(STATUS "library:                      ${MARVELL_LIBAV_LIBRARIES_AVCODEC} ${MARVELL_LIBAV_LIBRARIES_AVFORMAT} ${MARVELL_LIBAV_LIBRARIES_AVUTIL}")

if (MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY AND MARVELL_AMP_CLIENT_LIBRARIES AND
    MARVELL_OSAL_INCLUDE_DIRECTORY AND MARVELL_OSAL_LIBRARIES AND
    MARVELL_GFX_LIBRARIES AND
    MARVELL_LOG_LIBRARIES AND
    MARVELL_LIBAV_INCLUDE_DIRECTORY AND MARVELL_LIBAV_LIBRARIES_AVCODEC AND MARVELL_LIBAV_LIBRARIES_AVFORMAT AND MARVELL_LIBAV_LIBRARIES_AVUTIL)
    message(STATUS "Found Marvell SDK")
else()
    message(SEND_ERROR "Could not find Marvell SDK.")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MARVELL_AMP_CLIENT DEFAULT_MSG
    MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY MARVELL_AMP_CLIENT_LIBRARIES
    MARVELL_OSAL_INCLUDE_DIRECTORY MARVELL_OSAL_LIBRARIES
    MARVELL_GFX_LIBRARIES
    MARVELL_LOG_LIBRARIES
    MARVELL_LIBAV_INCLUDE_DIRECTORY MARVELL_LIBAV_LIBRARIES_AVCODEC MARVELL_LIBAV_LIBRARIES_AVFORMAT MARVELL_LIBAV_LIBRARIES_AVUTIL)

mark_as_advanced(
    MARVELL_AMP_CLIENT_INCLUDE_DIRECTORY MARVELL_AMP_CLIENT_LIBRARIES
    MARVELL_OSAL_INCLUDE_DIRECTORY MARVELL_OSAL_LIBRARIES
    MARVELL_GFX_LIBRARIES
    MARVELL_LOG_LIBRARIES
    MARVELL_LIBAV_INCLUDE_DIRECTORY MARVELL_LIBAV_LIBRARIES_AVCODEC MARVELL_LIBAV_LIBRARIES_AVFORMAT MARVELL_LIBAV_LIBRARIES_AVUTIL)