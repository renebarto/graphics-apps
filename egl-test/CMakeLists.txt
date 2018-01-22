project(egl-test)

set(CMAKE_CXX_STANDARD 11)

include(display_list)
include(show_target_properties)

if(X86_64_PLATFORM)
    add_subdirectory(x86_64/drm-info)
    add_subdirectory(x86_64/gbm-info)
endif()

set(EGL_TEST_PACKAGE_NAME ${PROJECT_NAME})
set(EGL_TEST_TARGET_NAME ${EGL_TEST_PACKAGE_NAME})
set(EGL_TEST_PACKAGE_DESCRIPTION "Simple EGL application")
set(EGL_TEST_PACKAGE_VERSION_MAJOR 1)
set(EGL_TEST_PACKAGE_VERSION_MINOR 0)
set(EGL_TEST_PACKAGE_VERSION_MICRO 0)
set(EGL_TEST_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

message(STATUS "*** Setting up ${EGL_TEST_PACKAGE_NAME} ***")
find_package(EGL REQUIRED)
find_package(OpenGLES2 REQUIRED)
#find_package(Wayland REQUIRED)
#find_package(LibXkbCommon REQUIRED)
#find_package(LibFfi REQUIRED)
if(SYNAPTICS_PLATFORM)
    #    find_package(AmpClient REQUIRED)
elseif(RPI_PLATFORM)
    find_package(RPIBcmHost REQUIRED)
    find_package(RPIVcHostIf REQUIRED)
else()
endif()
#find_package(WesterosSimpleShell REQUIRED)

set(EGL_TEST_DEFINITIONS
    )

if(SYNAPTICS_PLATFORM)
    list(APPEND EGL_TEST_DEFINITIONS SYNAPTICS_PLATFORM EGL_API_FB)
elseif(RPI_PLATFORM)
    list(APPEND EGL_TEST_DEFINITIONS RPI_PLATFORM)
else()
    list(APPEND EGL_TEST_DEFINITIONS X86_64_PLATFORM)
endif()

set(EGL_TEST_INCLUDE_DIRS
    .
    ${OPENGLES2_INCLUDE_DIRECTORY}
    ${EGL_INCLUDE_DIRECTORY}
#    ${WAYLAND_CLIENT_INCLUDE_DIRECTORY}
#    ${LIB_XKB_COMMON_INCLUDE_DIRECTORY}
#    ${LIB_FFI_INCLUDE_DIRECTORY}
    )

if(SYNAPTICS_PLATFORM)
    list(APPEND EGL_TEST_INCLUDE_DIRS )
elseif(RPI_PLATFORM)
    list(APPEND EGL_TEST_INCLUDE_DIRS
        ${RPI_BCM_HOST_INCLUDE_DIRECTORY}
        ${VC_HOST_IF_INCLUDE_DIRECTORY})
else()
    list(APPEND EGL_TEST_INCLUDE_DIRS)
endif()

set(EGL_TEST_OPTIONS
    )

set(EGL_TEST_LINK_OPTIONS)

set(EGL_TEST_DEPENDENCIES
    )

if(SYNAPTICS_PLATFORM)
elseif(RPI_PLATFORM)
else()
    list(APPEND EGL_TEST_DEPENDENCIES
        drminfo
        gbminfo
        )
endif()
set(EGL_TEST_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${EGL_LIBRARIES}
    ${OPENGLES2_LIBRARIES}
#    ${LIB_FFI_LIBRARIES}
    ${EGL_TEST_DEPENDENCIES})

if(SYNAPTICS_PLATFORM)
    list(APPEND EGL_TEST_LIBS)
elseif(RPI_PLATFORM)
    list(APPEND EGL_TEST_LIBS
        ${RPI_BCM_HOST_LIBRARIES}
        ${VC_HOST_IF_LIBRARY})
else()
    list(APPEND EGL_TEST_LIBS
        )
endif()

file(GLOB EGL_TEST_INCLUDES *.h)

set(EGL_TEST_SOURCES
    main.cpp
    )

if(SYNAPTICS_PLATFORM)
    list(APPEND EGL_TEST_SOURCES
        synaptics/NativeDisplay-synaptics.cpp
        synaptics/NativeWindow-synaptics.cpp)
    list(APPEND EGL_TEST_INCLUDES
        synaptics/NativeDisplay-synaptics.h
        synaptics/NativeWindow-synaptics.h)
elseif(RPI_PLATFORM)
    list(APPEND EGL_TEST_SOURCES
        rpi/NativeDisplay-rpi.cpp
        rpi/NativeWindow-rpi.cpp)
    list(APPEND EGL_TEST_INCLUDES
        rpi/NativeDisplay-rpi.h
        rpi/NativeWindow-rpi.h)
else()
    list(APPEND EGL_TEST_SOURCES
        x86_64/NativeDisplay-x64.cpp
        x86_64/NativeWindow-x64.cpp)
    list(APPEND EGL_TEST_INCLUDES
        x86_64/NativeDisplay-x64.h
        x86_64/NativeWindow-x64.h)
endif()

set(EGL_TEST_INPUT
    ${EGL_TEST_SOURCES}
    ${EGL_TEST_INCLUDES}
    )

if(BUILD_REFERENCE)
    list(APPEND EGL_TEST_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if(VERBOSE_BUILD)
    display_list("Defines                     : " ${EGL_TEST_DEFINITIONS} )
    display_list("Compiler options            : " ${EGL_TEST_OPTIONS} )
    display_list("Source files                : " ${EGL_TEST_SOURCES} )
    display_list("Include files               : " ${EGL_TEST_INCLUDES} )
    display_list("Include dirs                : " ${EGL_TEST_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${EGL_TEST_LIBS} )
    display_list("Linker options              : " ${EGL_TEST_LINK_OPTIONS} )
    display_list("Dependencies                : " ${EGL_TEST_DEPENDENCIES} )
endif()

add_executable(${EGL_TEST_TARGET_NAME} ${EGL_TEST_INPUT})
target_compile_definitions(${EGL_TEST_TARGET_NAME} PRIVATE ${EGL_TEST_DEFINITIONS})
target_compile_options(${EGL_TEST_TARGET_NAME} PRIVATE ${EGL_TEST_OPTIONS})
target_include_directories(${EGL_TEST_TARGET_NAME} PUBLIC ${EGL_TEST_INCLUDE_DIRS})
target_link_libraries(${EGL_TEST_TARGET_NAME} ${EGL_TEST_LIBS})
list_to_string(EGL_TEST_LINK_OPTIONS EGL_TEST_LINK_OPTIONS_STRING)
if(NOT "${EGL_TEST_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${EGL_TEST_TARGET_NAME} PROPERTIES LINK_FLAGS "${EGL_TEST_LINK_OPTIONS_STRING}")
endif()

if(VERBOSE_BUILD)
    show_target_properties(${EGL_TEST_TARGET_NAME})
endif()
