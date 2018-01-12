#include <fcntl.h>
#include <stddef.h>
#include <gbm.h>
#include <stdlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <xf86drmMode.h>
#include <unistd.h>

static const EGLint context_client_version[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE };

static const EGLint egl_fb_config[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
    EGL_NONE };

static const char device_name[] = "/dev/dri/card0";

int main(void) {

// Open the video card device in read/write mode.
    int fd;
    fd = open(device_name, O_RDWR);

    if (fd < 0) {
/* Probably permissions error */
        printf("couldn't open %s, skipping\n", device_name);
        exit(EXIT_FAILURE);
    }

// Create a GBM device out of it.  Keep the fd around though, we'll still need it.
    struct gbm_device *gbm;
    gbm = gbm_create_device(fd);

    if (gbm == NULL) {
        printf("GBM Device Creation Failed");
        exit(EXIT_FAILURE);
    }

// Get an EGL display from the GBM device.
    EGLDisplay dpy;
    dpy = eglGetDisplay(gbm);

    if (dpy == EGL_NO_DISPLAY) {
        printf("Could not get EGL display");
        exit(EXIT_FAILURE);
    }

// Initialize the display. Major and minor are version numbers.
    EGLint major, minor;
    if (eglInitialize(dpy, &major, &minor) == EGL_FALSE) {
        printf("Could initialize EGL display");
        exit(EXIT_FAILURE);
    }

// Tell the EGL we will be using the GL ES API.
    EGLContext ctx;

    eglBindAPI(EGL_OPENGL_ES_API);

// Choose the right framebuffer definition matching our requirements.
    EGLConfig config;
    EGLint n;

    if (!eglChooseConfig(dpy, egl_fb_config, &config, 1, &n) || n != 1) {
        printf("Failed to choose EGL FB config\n");
        exit(EXIT_FAILURE);
    }

// Create an EGL context from the chosen config, and specify we want version 2 of GLES.
        ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, context_client_version);

    if (ctx == EGL_NO_CONTEXT) {
        printf("Could not create context");
        exit(EXIT_FAILURE);
    }

    drmModeRes *res;
    drmModeConnector *conn;
    uint32_t conn_id, width, height;
    drmModeModeInfo modeinfo;
    res = drmModeGetResources(fd);
    conn = drmModeGetConnector(fd, *res->connectors);
    conn_id = conn->connector_id;
    width = conn->modes[0].hdisplay;
    height = conn->modes[0].vdisplay;
    modeinfo = conn->modes[0];
    drmModeFreeConnector(conn);

// Create a GBM surface, and then get an EGL surface from that.
    EGLSurface surface;
    struct gbm_surface *gs;
    gs = gbm_surface_create(gbm, width, height, GBM_BO_FORMAT_ARGB8888,
                            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    surface = eglCreateWindowSurface(dpy, config, gs, NULL);

    if (eglMakeCurrent(dpy, surface, surface, ctx) == EGL_FALSE) {
        printf("Could not make context current");
    }

// Render stuff. For now just clear the screen with a color, so we can at least tell it works.
        glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(dpy, surface);

    struct gbm_bo *bo;
    uint32_t handle, stride;
    bo = gbm_surface_lock_front_buffer(gs);
    handle = gbm_bo_get_handle(bo).u32;
    stride = gbm_bo_get_stride(bo);

    printf("handle=%d, stride=%d\n", handle, stride);

    int ret;
    uint32_t drm_fb_id;
    ret = drmModeAddFB(fd, width, height, 0, 32, stride, handle, &drm_fb_id);
    if (ret) {
        printf("failed to create fb\n");
        exit(EXIT_FAILURE);
    }

    ret = drmModeSetCrtc(fd, *res->crtcs, drm_fb_id, 0, 0, &conn_id, 1, &modeinfo);
    if (ret) {
        printf("failed to set mode: %m\n");
    }

    sleep(2);

    gbm_surface_release_buffer(gs, bo);
    drmModeRmFB(fd, drm_fb_id);
    gbm_bo_destroy(bo);
    gbm_surface_destroy(gs);
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surface);
    eglTerminate(dpy);
    gbm_device_destroy(gbm);
    close(fd);
    return 0;
}
