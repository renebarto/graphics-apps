project(wayland-simple-egl)

find_package(Wayland REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(EGL REQUIRED)
find_package(OpenGLES2 REQUIRED)

# To build weston:
#  x11proto-dri3-dev
#  x11proto-present-dev
#  libomxil-bellagio-dev
#  libxcb-composite0-dev
#  libxcursor-dev
#  libcairo2-dev
#  libgbm-dev
#  libmtdev-dev
#  libinput-dev
#  wayland-1.13.0 (from source)
#    ./configure --disable-documentation --prefix=/usr
#    make
#    sudo make install
#  python-mako
#  mesa-17.0.4
#    ./autogen.sh --prefix=/usr --with-gallium-drivers=swrast,r600,svga
#       --enable-gles2 --with-egl-platforms=x11,wayland,drm --enable-gbm --enable-shared-glapi
#  libffi-dev
#  libxml2-dev
#  wayland-protocols-1.7
#    ./configure --prefix=/usr
#    make
#    sudo make install
#  libpam0g-dev
#  weston-2.0.0
#    ./configure --prefix=/usr
#    make
#    sudo make install


if (NOT WAYLAND_CLIENT_FOUND)
    message(FATAL_ERROR "Could not find wayland-client")
else()
    message(STATUS "Wayland client: ${WAYLAND_CLIENT_LIBRARIES}")
endif()
if (NOT WAYLAND_CURSOR_FOUND)
    message(FATAL_ERROR "Could not find wayland-cursor")
else()
    message(STATUS "Wayland cursor: ${WAYLAND_CURSOR_LIBRARIES}")
endif()

message(STATUS ${WAYLAND_CLIENT_INCLUDE_DIR})
message(STATUS ${WAYLAND_CLIENT_LIBRARIES})

set(SOURCE_FILES
        simple-egl.c protocol/xdg-shell-unstable-v6-protocol.c protocol/ivi-application-protocol.c)
set(HEADER_FILES
        )
set(DATA_FILES
        )

message(STATUS ${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME}
    ${WAYLAND_CLIENT_LIBRARIES}
    ${WAYLAND_CURSOR_LIBRARIES}
    ${WAYLAND_EGL_LIBRARIES}
    ${EGL_LIBRARIES}
    ${OPENGLES2_LIBRARIES}
    m)

#add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${DATA_FILES} $<TARGET_FILE_DIR:${PROJECT_NAME}>)