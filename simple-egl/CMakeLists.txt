project(simple-egl)

set(CMAKE_CXX_STANDARD 11)

include(display_list)
include(show_target_properties)

find_package(EGL REQUIRED)
find_package(OpenGLES2 REQUIRED)
find_package(LibDRM REQUIRED)
find_package(GBM REQUIRED)

set(SIMPLE_EGL_PACKAGE_NAME ${PROJECT_NAME})
set(SIMPLE_EGL_TARGET_NAME ${SIMPLE_EGL_PACKAGE_NAME})
set(SIMPLE_EGL_PACKAGE_DESCRIPTION "Simple EGL application")
set(SIMPLE_EGL_PACKAGE_VERSION_MAJOR 1)
set(SIMPLE_EGL_PACKAGE_VERSION_MINOR 0)
set(SIMPLE_EGL_PACKAGE_VERSION_MICRO 0)
set(SIMPLE_EGL_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

set(SIMPLE_EGL_DEFINITIONS
    )

set(SIMPLE_EGL_INCLUDE_DIRS
    ${GBM_INCLUDE_DIRECTORY}
    ${LIB_DRM_INCLUDE_DIRECTORY}
    ${EGL_INCLUDE_DIRECTORY})

set(SIMPLE_EGL_OPTIONS
    )

set(SIMPLE_EGL_LINK_OPTIONS)

set(SIMPLE_EGL_DEPENDENCIES
    ${LIB_DRM_INFO_TARGET_NAME})

set(SIMPLE_EGL_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${EGL_LIBRARIES}
    ${OPENGLES2_LIBRARIES}
    GLU
    ${GBM_LIBRARIES}
    ${LIB_DRM_LIBRARIES}
    ${SIMPLE_EGL_DEPENDENCIES})

file(GLOB SIMPLE_EGL_INCLUDES *.h)

set(SIMPLE_EGL_SOURCES
    main.cpp
    )

set(SIMPLE_EGL_INPUT
    ${SIMPLE_EGL_SOURCES}
    ${SIMPLE_EGL_INCLUDES}
    )

if (BUILD_REFERENCE)
    list(APPEND SIMPLE_EGL_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if (VERBOSE_BUILD)
    display_list("Defines                     : " ${SIMPLE_EGL_DEFINITIONS} )
    display_list("Compiler options            : " ${SIMPLE_EGL_OPTIONS} )
    display_list("Source files                : " ${SIMPLE_EGL_SOURCES} )
    display_list("Include files               : " ${SIMPLE_EGL_INCLUDES} )
    display_list("Include dirs                : " ${SIMPLE_EGL_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${SIMPLE_EGL_LIBS} )
    display_list("Linker options              : " ${SIMPLE_EGL_LINK_OPTIONS} )
    display_list("Dependencies                : " ${SIMPLE_EGL_DEPENDENCIES} )
endif()

add_executable(${SIMPLE_EGL_TARGET_NAME} ${SIMPLE_EGL_INPUT})
target_compile_definitions(${SIMPLE_EGL_TARGET_NAME} PRIVATE ${SIMPLE_EGL_DEFINITIONS})
target_include_directories(${SIMPLE_EGL_TARGET_NAME} PUBLIC ${SIMPLE_EGL_INCLUDE_DIRS})
target_link_libraries(${SIMPLE_EGL_TARGET_NAME} ${SIMPLE_EGL_LIBS})
list_to_string(SIMPLE_EGL_LINK_OPTIONS SIMPLE_EGL_LINK_OPTIONS_STRING)
if (NOT "${SIMPLE_EGL_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${SIMPLE_EGL_TARGET_NAME} PROPERTIES LINK_FLAGS "${SIMPLE_EGL_LINK_OPTIONS_STRING}")
endif()

if (VERBOSE_BUILD)
    show_target_properties(${SIMPLE_EGL_TARGET_NAME})
endif()
