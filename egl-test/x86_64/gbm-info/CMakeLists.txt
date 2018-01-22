project(gbm-info)

set(CMAKE_CXX_STANDARD 11)

include(display_list)
include(show_target_properties)

set(LIB_GBM_INFO_PACKAGE_NAME gbminfo)
set(LIB_GBM_INFO_TARGET_NAME ${LIB_GBM_INFO_PACKAGE_NAME})
set(LIB_GBM_INFO_PACKAGE_DESCRIPTION "Gbm info library")
set(LIB_GBM_INFO_PACKAGE_VERSION_MAJOR 1)
set(LIB_GBM_INFO_PACKAGE_VERSION_MINOR 0)
set(LIB_GBM_INFO_PACKAGE_VERSION_MICRO 0)
set(LIB_GBM_INFO_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

message(STATUS "*** Setting up ${LIB_GBM_INFO_PACKAGE_NAME} ***")

find_package(GBM REQUIRED)

set(LIB_GBM_INFO_DEFINITIONS
    )

set(LIB_GBM_INFO_INCLUDE_DIRS
    ${GBM_INCLUDE_DIRECTORY}
    ../drm-info)

set(LIB_GBM_INFO_OPTIONS
    )

set(LIB_GBM_INFO_LINK_OPTIONS)

set(LIB_GBM_INFO_DEPENDENCIES
    drminfo)

set(LIB_GBM_INFO_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${GBM_LIBRARIES}
    ${LIB_GBM_INFO_DEPENDENCIES})

file(GLOB LIB_GBM_INFO_INCLUDES *.h)

set(LIB_GBM_INFO_SOURCES
    GbmInfo.cpp
    )

set(LIB_GBM_INFO_INPUT
    ${LIB_GBM_INFO_SOURCES}
    ${LIB_GBM_INFO_INCLUDES}
    )

if (BUILD_REFERENCE)
    list(APPEND LIB_GBM_INFO_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if (VERBOSE_BUILD)
    display_list("Defines                     : " ${LIB_GBM_INFO_DEFINITIONS} )
    display_list("Compiler options            : " ${LIB_GBM_INFO_OPTIONS} )
    display_list("Source files                : " ${LIB_GBM_INFO_SOURCES} )
    display_list("Include files               : " ${LIB_GBM_INFO_INCLUDES} )
    display_list("Include dirs                : " ${LIB_GBM_INFO_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${LIB_GBM_INFO_LIBS} )
    display_list("Linker options              : " ${LIB_GBM_INFO_LINK_OPTIONS} )
    display_list("Dependencies                : " ${LIB_GBM_INFO_DEPENDENCIES} )
endif()

add_library(${LIB_GBM_INFO_TARGET_NAME} STATIC ${LIB_GBM_INFO_INPUT})
target_compile_definitions(${LIB_GBM_INFO_TARGET_NAME} PRIVATE ${LIB_GBM_INFO_DEFINITIONS})
target_include_directories(${LIB_GBM_INFO_TARGET_NAME} PUBLIC ${LIB_GBM_INFO_INCLUDE_DIRS})
target_link_libraries(${LIB_GBM_INFO_TARGET_NAME} PUBLIC ${LIB_GBM_INFO_LIBS})
list_to_string(LIB_GBM_INFO_LINK_OPTIONS LIB_GBM_INFO_LINK_OPTIONS_STRING)
if (NOT "${LIB_GBM_INFO_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${LIB_GBM_INFO_TARGET_NAME} PROPERTIES LINK_FLAGS "${LIB_GBM_INFO_LINK_OPTIONS_STRING}")
endif()

if (VERBOSE_BUILD)
    show_target_properties(${LIB_GBM_INFO_TARGET_NAME})
endif()

set(GBM_INFO_PACKAGE_NAME ${PROJECT_NAME})
set(GBM_INFO_TARGET_NAME ${GBM_INFO_PACKAGE_NAME})
set(GBM_INFO_PACKAGE_DESCRIPTION "Drm info tool")
set(GBM_INFO_PACKAGE_VERSION_MAJOR 1)
set(GBM_INFO_PACKAGE_VERSION_MINOR 0)
set(GBM_INFO_PACKAGE_VERSION_MICRO 0)
set(GBM_INFO_PACKAGE_VERSION ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO})

message(STATUS "*** Setting up ${GBM_INFO_TARGET_NAME} ***")

set(GBM_INFO_DEFINITIONS
    )

set(GBM_INFO_INCLUDE_DIRS
    )

set(GBM_INFO_OPTIONS
    )

set(GBM_INFO_LINK_OPTIONS)

set(GBM_INFO_DEPENDENCIES
    ${LIB_GBM_INFO_TARGET_NAME})

set(GBM_INFO_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${GBM_INFO_DEPENDENCIES})

file(GLOB GBM_INFO_INCLUDES *.h)

set(GBM_INFO_SOURCES
    main.cpp
    )

set(GBM_INFO_INPUT
    ${GBM_INFO_SOURCES}
    ${GBM_INFO_INCLUDES}
    )

if (BUILD_REFERENCE)
    list(APPEND GBM_INFO_DEFINITIONS BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if (VERBOSE_BUILD)
    display_list("Defines                     : " ${GBM_INFO_DEFINITIONS} )
    display_list("Compiler options            : " ${GBM_INFO_OPTIONS} )
    display_list("Source files                : " ${GBM_INFO_SOURCES} )
    display_list("Include files               : " ${GBM_INFO_INCLUDES} )
    display_list("Include dirs                : " ${GBM_INFO_INCLUDE_DIRS} )
    display_list("Link libs                   : " ${GBM_INFO_LIBS} )
    display_list("Linker options              : " ${GBM_INFO_LINK_OPTIONS} )
    display_list("Dependencies                : " ${GBM_INFO_DEPENDENCIES} )
endif()

add_executable(${GBM_INFO_TARGET_NAME} ${GBM_INFO_INPUT})
target_compile_definitions(${GBM_INFO_TARGET_NAME} PRIVATE ${GBM_INFO_DEFINITIONS})
target_include_directories(${GBM_INFO_TARGET_NAME} PUBLIC ${GBM_INFO_INCLUDE_DIRS})
target_link_libraries(${GBM_INFO_TARGET_NAME} ${GBM_INFO_LIBS})
list_to_string(GBM_INFO_LINK_OPTIONS GBM_INFO_LINK_OPTIONS_STRING)
if (NOT "${GBM_INFO_LINK_OPTIONS_STRING}" STREQUAL "")
    set_target_properties(${GBM_INFO_TARGET_NAME} PROPERTIES LINK_FLAGS "${GBM_INFO_LINK_OPTIONS_STRING}")
endif()

if (VERBOSE_BUILD)
    show_target_properties(${GBM_INFO_TARGET_NAME})
endif()
