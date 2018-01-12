project(drm-info)

set(CMAKE_CXX_STANDARD 11)

include(display_list)
include(show_target_properties)

find_package(LibDRM REQUIRED)

set(LIB_DRM_INFO_PACKAGE_NAME drminfo)
set(LIB_DRM_INFO_TARGET_NAME ${LIB_DRM_INFO_PACKAGE_NAME})
set(LIB_DRM_INFO_PACKAGE_DESCRIPTION "Drm info library")
set(LIB_DRM_INFO_PACKAGE_VERSION_MAJOR 1)
set(LIB_DRM_INFO_PACKAGE_VERSION_MINOR 0)
set(LIB_DRM_INFO_PACKAGE_VERSION_MICRO 0)
set(LIB_DRM_INFO_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

set(LIB_DRM_INFO_DEFINITIONS
    )

set(LIB_DRM_INFO_INCLUDE_DIRS
    ${LIB_DRM_INCLUDE_DIRECTORY})

set(LIB_DRM_INFO_OPTIONS
    )

set(LIB_DRM_INFO_LINK_OPTIONS)

set(LIB_DRM_INFO_DEPENDENCIES
    )

set(LIB_DRM_INFO_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${LIB_DRM_LIBRARIES}
    ${LIB_DRM_INFO_DEPENDENCIES})

file(GLOB LIB_DRM_INFO_INCLUDES *.h)

set(LIB_DRM_INFO_SOURCES
    DrmInfo.cpp
    )

set(LIB_DRM_INFO_INPUT
    ${LIB_DRM_INFO_SOURCES}
    ${LIB_DRM_INFO_INCLUDES}
    )

if (BUILD_REFERENCE)
    list(APPEND LIB_DRM_INFO_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if (VERBOSE_BUILD)
    display_list("Defines                     : " ${LIB_DRM_INFO_DEFINITIONS} )
    display_list("Compiler options            : " ${LIB_DRM_INFO_OPTIONS} )
    display_list("Source files                : " ${LIB_DRM_INFO_SOURCES} )
    display_list("Include files               : " ${LIB_DRM_INFO_INCLUDES} )
    display_list("Include dirs                : " ${LIB_DRM_INFO_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${LIB_DRM_INFO_LIBS} )
    display_list("Linker options              : " ${LIB_DRM_INFO_LINK_OPTIONS} )
    display_list("Dependencies                : " ${LIB_DRM_INFO_DEPENDENCIES} )
endif()

add_library(${LIB_DRM_INFO_TARGET_NAME} STATIC ${LIB_DRM_INFO_INPUT})
target_compile_definitions(${LIB_DRM_INFO_TARGET_NAME} PRIVATE ${LIB_DRM_INFO_DEFINITIONS})
target_include_directories(${LIB_DRM_INFO_TARGET_NAME} PUBLIC ${LIB_DRM_INFO_INCLUDE_DIRS})
target_link_libraries(${LIB_DRM_INFO_TARGET_NAME} ${LIB_DRM_INFO_LIBS})
list_to_string(LIB_DRM_INFO_LINK_OPTIONS LIB_DRM_INFO_LINK_OPTIONS_STRING)
if (NOT "${LIB_DRM_INFO_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${LIB_DRM_INFO_TARGET_NAME} PROPERTIES LINK_FLAGS "${LIB_DRM_INFO_LINK_OPTIONS_STRING}")
endif()

if (VERBOSE_BUILD)
    show_target_properties(${LIB_DRM_INFO_TARGET_NAME})
endif()

set(DRM_INFO_PACKAGE_NAME ${PROJECT_NAME})
set(DRM_INFO_TARGET_NAME ${DRM_INFO_PACKAGE_NAME})
set(DRM_INFO_PACKAGE_DESCRIPTION "Drm info tool")
set(DRM_INFO_PACKAGE_VERSION_MAJOR 1)
set(DRM_INFO_PACKAGE_VERSION_MINOR 0)
set(DRM_INFO_PACKAGE_VERSION_MICRO 0)
set(DRM_INFO_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

set(DRM_INFO_DEFINITIONS
    )

set(DRM_INFO_INCLUDE_DIRS
    )

set(DRM_INFO_OPTIONS
    )

set(DRM_INFO_LINK_OPTIONS)

set(DRM_INFO_DEPENDENCIES
    ${LIB_DRM_INFO_TARGET_NAME})

set(DRM_INFO_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${DRM_INFO_DEPENDENCIES})

file(GLOB DRM_INFO_INCLUDES *.h)

set(DRM_INFO_SOURCES
    main.cpp
    )

set(DRM_INFO_INPUT
    ${DRM_INFO_SOURCES}
    ${DRM_INFO_INCLUDES}
    )

if (BUILD_REFERENCE)
    list(APPEND DRM_INFO_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if (VERBOSE_BUILD)
    display_list("Defines                     : " ${DRM_INFO_DEFINITIONS} )
    display_list("Compiler options            : " ${DRM_INFO_OPTIONS} )
    display_list("Source files                : " ${DRM_INFO_SOURCES} )
    display_list("Include files               : " ${DRM_INFO_INCLUDES} )
    display_list("Include dirs                : " ${DRM_INFO_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${DRM_INFO_LIBS} )
    display_list("Linker options              : " ${DRM_INFO_LINK_OPTIONS} )
    display_list("Dependencies                : " ${DRM_INFO_DEPENDENCIES} )
endif()

add_executable(${DRM_INFO_TARGET_NAME} ${DRM_INFO_INPUT})
target_compile_definitions(${DRM_INFO_TARGET_NAME} PRIVATE ${DRM_INFO_DEFINITIONS})
target_include_directories(${DRM_INFO_TARGET_NAME} PUBLIC ${DRM_INFO_INCLUDE_DIRS})
target_link_libraries(${DRM_INFO_TARGET_NAME} ${DRM_INFO_LIBS})
list_to_string(DRM_INFO_LINK_OPTIONS DRM_INFO_LINK_OPTIONS_STRING)
if (NOT "${DRM_INFO_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${DRM_INFO_TARGET_NAME} PROPERTIES LINK_FLAGS "${DRM_INFO_LINK_OPTIONS_STRING}")
endif()

if (VERBOSE_BUILD)
    show_target_properties(${DRM_INFO_TARGET_NAME})
endif()
