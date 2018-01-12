#include <fcntl.h>
#include <zconf.h>
#include <cassert>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GL/glu.h>
#include <xf86drmMode.h>

using namespace std;

struct display {
//    struct wl_display *display;
//    struct wl_registry *registry;
//    struct wl_compositor *compositor;
//    struct zxdg_shell_v6 *shell;
//    struct wl_seat *seat;
//    struct wl_pointer *pointer;
//    struct wl_touch *touch;
//    struct wl_keyboard *keyboard;
//    struct wl_shm *shm;
//    struct wl_cursor_theme *cursor_theme;
//    struct wl_cursor *default_cursor;
//    struct wl_surface *cursor_surface;
    struct {
        EGLDisplay dpy;
        EGLContext ctx;
        EGLConfig conf;
        EGLSurface surface;
    } egl;
    struct {
        int fd;
        struct gbm_device *gbm;
        struct gbm_surface *gs;
    } gbm;
    struct window *window;
//    struct ivi_application *ivi_application;

    PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;
};

struct geometry {
    int width, height;
};

struct window {
    struct display *display;
    struct geometry geometry, window_size;
    struct {
        GLuint rotation_uniform;
        GLuint pos;
        GLuint col;
    } gl;

    uint32_t benchmark_time, frames;
//    struct wl_egl_window *native;
//    struct wl_surface *surface;
//    struct zxdg_surface_v6 *xdg_surface;
//    struct zxdg_toplevel_v6 *xdg_toplevel;
//    struct ivi_surface *ivi_surface;
    EGLSurface egl_surface;
//    struct wl_callback *callback;
    int fullscreen, opaque, buffer_size, frame_sync, delay;
    bool wait_for_configure;
};

static int running = 1;
static const char DeviceName[] = "/dev/dri/card0";

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

static bool
CheckEGLExtension(const char *extensions, const char *extension)
{
    size_t extLen = strlen(extension);
    const char *end = extensions + strlen(extensions);

    while (extensions < end) {
        size_t n = 0;

        /* Skip whitespaces, if any */
        if (*extensions == ' ') {
            extensions++;
            continue;
        }

        n = strcspn(extensions, " ");

        /* Compare strings */
        if (n == extLen && strncmp(extension, extensions, n) == 0)
            return true; /* Found */

        extensions += n;
    }

    /* Not found */
    return false;
}

static void
init_egl(struct display *display, struct window *window)
{
    static const struct {
        const char *extension, *entrypoint;
    } swap_damage_ext_to_entrypoint[] = {
        {
            .extension = "EGL_EXT_swap_buffers_with_damage",
            .entrypoint = "eglSwapBuffersWithDamageEXT",
        },
        {
            .extension = "EGL_KHR_swap_buffers_with_damage",
            .entrypoint = "eglSwapBuffersWithDamageKHR",
        },
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    const char *extensions;

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint major, minor, n, count, i, size;
    EGLConfig *configs;
    EGLBoolean ret;

    if (window->opaque || window->buffer_size == 16)
        config_attribs[9] = 0;

    // Open the video card device in read/write mode.
    display->gbm.fd = open(DeviceName, O_RDWR);

    if (display->gbm.fd < 0) {
        // Probably permissions error
        cerr << "couldn't open " << DeviceName << ", skipping" << endl;
        exit(EXIT_FAILURE);
    }

    // Create a GBM device out of it.  Keep the fd around though, we'll still need it.
    display->gbm.gbm = gbm_create_device(display->gbm.fd);

    if (display->gbm.gbm == nullptr) {
        cerr << "GBM Device Creation Failed" << endl;
        exit(EXIT_FAILURE);
    }

    display->egl.dpy = eglGetDisplay(display->gbm.gbm);
    assert(display->egl.dpy);

    ret = eglInitialize(display->egl.dpy, &major, &minor);
    assert(ret == EGL_TRUE);
    ret = eglBindAPI(EGL_OPENGL_ES_API);
    assert(ret == EGL_TRUE);

    if (!eglGetConfigs(display->egl.dpy, NULL, 0, &count) || count < 1)
        assert(0);

    configs = reinterpret_cast<EGLConfig *>(calloc(count, sizeof *configs));
    assert(configs);

    ret = eglChooseConfig(display->egl.dpy, config_attribs,
                          configs, count, &n);
    assert(ret && n >= 1);

    for (i = 0; i < n; i++) {
        eglGetConfigAttrib(display->egl.dpy,
                           configs[i], EGL_BUFFER_SIZE, &size);
        if (window->buffer_size == size) {
            display->egl.conf = configs[i];
            break;
        }
    }
    free(configs);
    if (display->egl.conf == nullptr) {
        cerr << "did not find config with buffer size " << window->buffer_size << endl;
        exit(EXIT_FAILURE);
    }

    display->egl.ctx = eglCreateContext(display->egl.dpy,
                                        display->egl.conf,
                                        EGL_NO_CONTEXT, context_attribs);
    assert(display->egl.ctx);

    display->swap_buffers_with_damage = nullptr;
    extensions = eglQueryString(display->egl.dpy, EGL_EXTENSIONS);
    if (extensions &&
        CheckEGLExtension(extensions, "EGL_EXT_buffer_age")) {
        for (i = 0; i < (int) ARRAY_LENGTH(swap_damage_ext_to_entrypoint); i++) {
            if (CheckEGLExtension(extensions,
                                           swap_damage_ext_to_entrypoint[i].extension)) {
                /* The EXTPROC is identical to the KHR one */
                display->swap_buffers_with_damage =
                    (PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC)
                        eglGetProcAddress(swap_damage_ext_to_entrypoint[i].entrypoint);
                break;
            }
        }
    }

    if (display->swap_buffers_with_damage)
        cout << "has EGL_EXT_buffer_age and " << swap_damage_ext_to_entrypoint[i].extension << endl;

}

static void fini_egl(struct display *display)
{
    eglTerminate(display->egl.dpy);
    eglReleaseThread();
}

static const char *vert_shader_text =
    "uniform mat4 rotation;\n"
        "attribute vec4 pos;\n"
        "attribute vec4 color;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  gl_Position = rotation * pos;\n"
        "  v_color = color;\n"
        "}\n";

static const char *frag_shader_text =
    "precision mediump float;\n"
        "varying vec4 v_color;\n"
        "void main() {\n"
        "  gl_FragColor = v_color;\n"
        "}\n";

static GLuint create_shader(struct window *window, const char *source, GLenum shader_type)
{
    GLuint shader;
    GLint status;

    shader = glCreateShader(shader_type);
    if (shader == 0)
    {
        GLenum errorCode = glGetError();
        const GLubyte * errorString = gluErrorString(errorCode);
        cerr << "Cannot create shared: " << string((const char *)errorString);
    }
    assert(shader != 0);

    glShaderSource(shader, 1, (const char **) &source, nullptr);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetShaderInfoLog(shader, 1000, &len, log);
        cerr << "Error: compiling " << (shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment") << endl
             << string(log, len) << endl;
        exit(1);
    }

    return shader;
}
static void create_surface(struct display * display, struct window * window)
{
    drmModeRes *res;
    drmModeConnector *conn;
    uint32_t conn_id, width, height;
    drmModeModeInfo modeinfo;
    res = drmModeGetResources(display->gbm.fd);
    conn = drmModeGetConnector(display->gbm.fd, *res->connectors);

    conn_id = conn->connector_id;
//    width = conn->modes[0].hdisplay;
//    height = conn->modes[0].vdisplay;
//    modeinfo = conn->modes[0];
    drmModeFreeConnector(conn);

// Create a GBM surface, and then get an EGL surface from that.
    display->gbm.gs = gbm_surface_create(display->gbm.gbm, width, height, GBM_BO_FORMAT_ARGB8888,
                            GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    display->egl.surface = eglCreateWindowSurface(display->egl.dpy,
                                     display->egl.conf, display->gbm.gs, nullptr);
}

static void destroy_surface(struct display * display, struct window *window)
{
    gbm_surface_destroy(display->gbm.gs);
    /* Required, otherwise segfault in egl_dri2.c: dri2_make_current()
     * on eglReleaseThread(). */
    eglMakeCurrent(display->egl.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display->egl.dpy, display->egl.ctx);
    eglDestroySurface(display->egl.dpy, display->egl.surface);
    gbm_device_destroy(display->gbm.gbm);
    close(display->gbm.fd);
}

static void init_gl(struct window *window)
{
    GLuint frag, vert;
    GLuint program;
    GLint status;

    frag = create_shader(window, frag_shader_text, GL_FRAGMENT_SHADER);
    vert = create_shader(window, vert_shader_text, GL_VERTEX_SHADER);

    program = glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(program, 1000, &len, log);
        cerr << "Error: linking:" << endl
             << string(log, len) << endl;
        exit(1);
    }

    glUseProgram(program);

    window->gl.pos = 0;
    window->gl.col = 1;

    glBindAttribLocation(program, window->gl.pos, "pos");
    glBindAttribLocation(program, window->gl.col, "color");
    glLinkProgram(program);

    window->gl.rotation_uniform =
        glGetUniformLocation(program, "rotation");
}

static void signal_int(int signum)
{
    running = 0;
}

static void usage(int error_code)
{
    cerr << "Usage: simple-egl [OPTIONS]" << endl << endl <<
        "  -d <us>\tBuffer swap delay in microseconds" << endl <<
        "  -f\tRun in fullscreen mode" << endl <<
        "  -o\tCreate an opaque surface" << endl <<
        "  -s\tUse a 16 bpp EGL config" << endl <<
        "  -b\tDon't sync to compositor redraw (eglSwapInterval 0)" << endl <<
        "  -h\tThis help text" << endl << endl;

    exit(error_code);
}

int main(int argc, char **argv)
{
    struct sigaction sigint;
    struct display display = { 0 };
    struct window  window  = { 0 };
    int i, ret = 0;

    window.display = &display;
    display.window = &window;
    window.geometry.width  = 250;
    window.geometry.height = 250;
    window.window_size = window.geometry;
    window.buffer_size = 32;
    window.frame_sync = 1;
    window.delay = 0;

    for (i = 1; i < argc; i++) {
//        if (strcmp("-d", argv[i]) == 0 && i+1 < argc)
//            window.delay = atoi(argv[++i]);
//        else if (strcmp("-f", argv[i]) == 0)
//            window.fullscreen = 1;
//        else if (strcmp("-o", argv[i]) == 0)
//            window.opaque = 1;
//        else if (strcmp("-s", argv[i]) == 0)
//            window.buffer_size = 16;
//        else if (strcmp("-b", argv[i]) == 0)
//            window.frame_sync = 0;
        /*else*/ if (strcmp("-h", argv[i]) == 0)
            usage(EXIT_SUCCESS);
        else
            usage(EXIT_FAILURE);
    }

//    display.display = wl_display_connect(nullptr);
//    assert(display.display);
//
//    display.registry = wl_display_get_registry(display.display);
//    wl_registry_add_listener(display.registry,
//                             &registry_listener, &display);
//
//    wl_display_roundtrip(display.display);

    init_egl(&display, &window);
    create_surface(&display, &window);
    init_gl(&window);

//    display.cursor_surface =
//        wl_compositor_create_surface(display.compositor);

    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, nullptr);

    /* The mainloop here is a little subtle.  Redrawing will cause
     * EGL to read events so we can just call
     * wl_display_dispatch_pending() to handle any events that got
     * queued up as a side effect. */
//    while (running && ret != -1) {
//        if (window.wait_for_configure) {
//            wl_display_dispatch(display.display);
//        } else {
//            wl_display_dispatch_pending(display.display);
//            redraw(&window, nullptr, 0);
//        }
//    }

    cerr << "simple-egl exiting" << endl;

    destroy_surface(&display, &window);
    fini_egl(&display);

//    wl_surface_destroy(display.cursor_surface);
//    if (display.cursor_theme)
//        wl_cursor_theme_destroy(display.cursor_theme);
//
//    if (display.shell)
//        zxdg_shell_v6_destroy(display.shell);
//
//    if (display.ivi_application)
//        ivi_application_destroy(display.ivi_application);
//
//    if (display.compositor)
//        wl_compositor_destroy(display.compositor);
//
//    wl_registry_destroy(display.registry);
//    wl_display_flush(display.display);
//    wl_display_disconnect(display.display);

    return 0;
}
