/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <iostream>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include "NativeDisplay.h"
#include "NativeWindow.h"

using namespace std;
#if defined(SYNAPTICS_PLATFORM)
#define DEVICE_ID nullptr
#elif defined(RPI_PLATFORM)
#define DEVICE_ID 0
#elif defined(X86_64_PLATFORM)
#define DEVICE_ID 0
#else
#error "Unsupported platform"
#endif

typedef struct _AppCtx
{
    NativeDisplay * nativeDisplay {};
    NativeWindow * nativeWindow {};
    void * native;

    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLSurface eglSurfaceWindow;
    EGLContext eglContext;
    EGLImageKHR eglImage;
    EGLNativePixmapType eglPixmap;

    int planeX;
    int planeY;
    int planeWidth;
    int planeHeight;

    uint32_t surfaceIdOther;
    uint32_t surfaceIdCurrent;
    float surfaceOpacity;
    float surfaceZOrder;
    bool surfaceVisible;
    int surfaceX;
    int surfaceY;
    int surfaceWidth;
    int surfaceHeight;

    int surfaceDX;
    int surfaceDY;
    int surfaceDWidth;
    int surfaceDHeight;

    struct
    {
        GLuint rotation_uniform;
        GLuint pos;
        GLuint col;
    } gl;
    long long startTime;
    long long currTime;
    bool noAnimation;
    bool needRedraw;
    bool verboseLog;
    int pointerX, pointerY;
} AppCtx;


static bool setupEGL(AppCtx *ctx);
static void termEGL(AppCtx *ctx);
static bool createSurface(AppCtx *ctx);
static void resizeSurface( AppCtx *ctx, int dx, int dy, int width, int height );
static void destroySurface(AppCtx *ctx);
static bool setupGL(AppCtx *ctx);
static bool renderGL(AppCtx *ctx);

int g_running= 0;
int g_log= 0;

static void signalHandler(int signum)
{
    cout << "signalHandler: signum " << signum << endl;
    g_running = 0;
}

static long long currentTimeMillis()
{
    long long timeMillis;
    struct timeval tv;

    gettimeofday(&tv, nullptr);
    timeMillis = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return timeMillis;
}

static void showUsage()
{
    cout << "usage:" << endl;
    cout << " egl-test [options]" << endl;
    cout << "where [options] are:" << endl;
    cout << "  --delay <delay> : render loop delay" << endl;
    cout << "  --shell : use wl_simple_shell protocol" << endl;
    cout << "  --display <name> : wayland display to connect to" << endl;
    cout << "  --noframe : don't pace rendering with frame requests" << endl;
    cout << "  --noanimate : don't use animation" << endl;
    cout << "  -? : show usage" << endl;
    cout << endl;
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    AppCtx *ctx= (AppCtx*)data;

    if (g_log)
        cout << "redraw: time " << time << endl;

    ctx->needRedraw= true;
}

int main( int argc, char** argv)
{
    int nRC= 0;
    AppCtx ctx;
    struct sigaction sigint;
    struct wl_registry *registry= 0;
    int count;
    int delay= 16667;
    const char *display_name= 0;
    EGLBoolean swapok;

    cout << "egl-test: v1.0" << endl;

    memset( &ctx, 0, sizeof(AppCtx) );

    for (int i= 1; i < argc; ++i)
    {
        if (!strcmp( (const char*)argv[i], "--delay"))
        {
            cout << "got delay: i " << i << " argc " << argc << endl;
            if (i + 1 < argc)
            {
                int v= atoi(argv[++i]);
                cout << "v=" << v << endl;
                if (v > 0)
                {
                    delay= v;
                    cout << "using delay=" << delay << endl;
                }
            }
        }
        else if (!strcmp((const char*)argv[i], "--display"))
        {
            if (i + 1 < argc)
            {
                ++i;
                display_name = argv[i];
            }
        }
        else if (!strcmp((const char*)argv[i], "--verbose"))
        {
            ctx.verboseLog = true;
        }
        else if (!strcmp((const char*)argv[i], "--log"))
        {
            g_log= true;
        }
        else if (!strcmp((const char*)argv[i], "--noanimate"))
        {
            ctx.noAnimation = true;
        }
        else if (!strcmp((const char*)argv[i], "-?"))
        {
            showUsage();
            goto exit;
        }
    }

    ctx.startTime= currentTimeMillis();

    if (display_name)
    {
        cout << "calling wl_display_connect for display name " << display_name << endl;
    }
    else
    {
        cout << "calling wl_display_connect for default display" << endl;
    }

    ctx.nativeDisplay = CreateDisplay(DEVICE_ID);
    cout << "display " << ctx.nativeDisplay << endl;
    if (ctx.nativeDisplay)
    {
        if (ctx.nativeDisplay->Open())
        {
            cout << "opened display, handle = " << hex << ctx.nativeDisplay->GetHandle() << dec << endl;
        } else
        {
            cerr << "could not open display" << endl;
            goto exit;
        }
    }

    ctx.planeX= 0;
    ctx.planeY= 0;
    ctx.planeWidth= 1280;
    ctx.planeHeight= 720;

    setupEGL(&ctx);

    ctx.surfaceWidth= 1280;
    ctx.surfaceHeight= 720;
    ctx.surfaceX= 0;
    ctx.surfaceY= 0;

    createSurface(&ctx);
    if (ctx.nativeWindow && ctx.eglSurfaceWindow)
    {
        cout << "Created window, handle " << ctx.nativeWindow->GetHandle() << " EGL surface " << ctx.eglSurfaceWindow << endl;
    } else
    {
        cerr << "Failed to create window" << endl;
        goto exit;
    }
    setupGL(&ctx);

    sigint.sa_handler = signalHandler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, nullptr);

    g_running= 1;
    while( g_running )
    {
        if ( delay > 0 )
        {
            usleep(delay);
        }
        renderGL(&ctx);
        eglSwapBuffers(ctx.eglDisplay, ctx.eglSurfaceWindow);
    }

exit:

    cout << "egl-test: exiting..." << endl;

    termEGL(&ctx);

    cout << "egl-test: exit" << endl;

    return nRC;
}

#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (0)

static bool setupEGL(AppCtx *ctx)
{
    bool result = false;
    EGLConfig * eglConfigs = nullptr;
    EGLBoolean eglResult = false;

    ctx->eglDisplay = eglGetDisplay(ctx->nativeDisplay->GetHandle());
    
    cout << "eglDisplay=" << ctx->eglDisplay << endl;
    if ( ctx->eglDisplay == EGL_NO_DISPLAY )
    {
        cerr << "error: EGL not available" << endl;
        goto exit;
    }

    /*
     * Initialize display
     */
    EGLint major, minor;
    eglResult = eglInitialize(ctx->eglDisplay, &major, &minor);
    if (!eglResult)
    {
        cerr << "error: unable to initialize EGL display" << endl;
        goto exit;
    }
    cout << "eglInitialize: major: " << major << " minor: " << minor << endl;

    /*
     * Get number of available configurations
     */
    EGLint configCount;
    eglResult = eglGetConfigs(ctx->eglDisplay, nullptr, 0, &configCount);
    if (!eglResult)
    {
        cerr << "error: unable to get count of EGL configurations: " << eglGetError() << endl;
        goto exit;
    }
    cout << "Number of EGL configurations: " << configCount << endl;

    eglConfigs= (EGLConfig*)malloc(configCount*sizeof(EGLConfig));
    if (!eglConfigs)
    {
        cerr << "error: unable to alloc memory for EGL configurations" << endl;
        goto exit;
    }

    EGLint attr[32];
    {
        int i = 0;
        attr[i++] = EGL_RED_SIZE;
        attr[i++] = RED_SIZE;
        attr[i++] = EGL_GREEN_SIZE;
        attr[i++] = GREEN_SIZE;
        attr[i++] = EGL_BLUE_SIZE;
        attr[i++] = BLUE_SIZE;
        attr[i++] = EGL_DEPTH_SIZE;
        attr[i++] = DEPTH_SIZE;
        attr[i++] = EGL_STENCIL_SIZE;
        attr[i++] = 0;
        attr[i++] = EGL_SURFACE_TYPE;
        attr[i++] = EGL_WINDOW_BIT;
        attr[i++] = EGL_RENDERABLE_TYPE;
        attr[i++] = EGL_OPENGL_ES2_BIT;
        attr[i++] = EGL_NONE;
    }

    /*
     * Get a list of configurations that meet or exceed our requirements
     */
    eglResult = eglChooseConfig(ctx->eglDisplay, attr, eglConfigs, configCount, &configCount);
    if (!eglResult)
    {
        cout << "error: eglChooseConfig failed: " << eglGetError() << endl;
        goto exit;
    }
    cout << "eglChooseConfig: matching configurations: " << configCount << endl;

    /*
     * Choose a suitable configuration
     */
    int i;
    for (i = 0; i < configCount; ++i)
    {
        EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
        eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_RED_SIZE, &redSize );
        eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_GREEN_SIZE, &greenSize );
        eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_BLUE_SIZE, &blueSize );
        eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_ALPHA_SIZE, &alphaSize );
        eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_DEPTH_SIZE, &depthSize );

        cout << "config " << i << ": red: " << redSize << " green: " << greenSize << " blue: " << blueSize
             << " alpha: " << alphaSize << " depth: " << depthSize << endl;
        if ( (redSize == RED_SIZE) &&
             (greenSize == GREEN_SIZE) &&
             (blueSize == BLUE_SIZE) &&
             (alphaSize == ALPHA_SIZE) &&
             (depthSize >= DEPTH_SIZE) )
        {
            cout <<  "choosing config " << i << endl;
            break;
        }
    }
    if (i == configCount)
    {
        cerr << "error: no suitable configuration available" << endl;
        goto exit;
    }
    ctx->eglConfig = eglConfigs[i];

    EGLint ctxAttrib[3];
    ctxAttrib[0]= EGL_CONTEXT_CLIENT_VERSION;
    ctxAttrib[1]= 2; // ES2
    ctxAttrib[2]= EGL_NONE;

    /*
     * Create an EGL context
     */
    ctx->eglContext = eglCreateContext(ctx->eglDisplay, ctx->eglConfig, EGL_NO_CONTEXT, ctxAttrib);
    if (ctx->eglContext == EGL_NO_CONTEXT)
    {
        cerr <<  "eglCreateContext failed: " << eglGetError() << endl;
        goto exit;
    }
    cout << "eglCreateContext: eglContext " << ctx->eglContext << endl;

    result = true;

    exit:

    if (eglConfigs)
    {
        free(eglConfigs);
        eglConfigs = nullptr;
    }

    return result;
}

static void termEGL(AppCtx *ctx)
{
    if (ctx->eglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(ctx->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        destroySurface(ctx);

        cout << "Destroy EGL context" << endl;
        eglDestroyContext(ctx->eglDisplay, ctx->eglContext);
        cout << "Terminate EGL" << endl;
        eglTerminate(ctx->eglDisplay);
        cout << "Release EGL thread" << endl;
        eglReleaseThread();
        if (ctx->nativeDisplay)
        {
            cout << "Close display" << endl;
            ctx->nativeDisplay->Close();
            delete ctx->nativeDisplay;
            ctx->nativeDisplay = nullptr;
        }
    }
}

static bool createSurface(AppCtx *ctx)
{
    bool result= false;
    EGLBoolean b;

    Size displaySize = ctx->nativeDisplay->GetSize();
    Rect rect {0, 0, displaySize.width, displaySize.height};

    ctx->nativeWindow = CreateWindow(ctx->nativeDisplay->GetContext(), rect);

    if (!ctx->nativeWindow)
    {
        ctx->eglSurfaceWindow = EGL_NO_SURFACE;
        return false;
    }

    /*
     * Create a window surface
     */
    ctx->eglSurfaceWindow= eglCreateWindowSurface(ctx->eglDisplay,
                                                  ctx->eglConfig,
                                                  ctx->nativeWindow->GetHandle(),
                                                  nullptr);
    if (ctx->eglSurfaceWindow == EGL_NO_SURFACE)
    {
        cerr << "eglCreateWindowSurface: A: error " << eglGetError() << endl;
        ctx->eglSurfaceWindow= eglCreateWindowSurface( ctx->eglDisplay,
                                                       ctx->eglConfig,
                                                       (EGLNativeWindowType)nullptr,
                                                       nullptr);
        if (ctx->eglSurfaceWindow == EGL_NO_SURFACE)
        {
            cerr << "eglCreateWindowSurface: B: error " << eglGetError() << endl;
            goto exit;
        }
    }
    cout << "eglCreateWindowSurface: eglSurfaceWindow " << ctx->eglSurfaceWindow << endl;

    /*
     * Establish EGL context for this thread
     */
    b= eglMakeCurrent(ctx->eglDisplay, ctx->eglSurfaceWindow, ctx->eglSurfaceWindow, ctx->eglContext);
    if ( !b )
    {
        cout << "error: eglMakeCurrent failed: " << eglGetError() << endl;
        goto exit;
    }

    eglSwapInterval( ctx->eglDisplay, 1 );

    exit:

    return result;
}

static void destroySurface(AppCtx *ctx)
{
    if (ctx->eglSurfaceWindow)
    {
        eglDestroySurface(ctx->eglDisplay, ctx->eglSurfaceWindow);

        if (ctx->nativeWindow)
        {
            ctx->nativeWindow->Destroy();
            delete ctx->nativeWindow;
            ctx->nativeWindow = nullptr;
        }
        ctx->eglSurfaceWindow = nullptr;
    }
}

static void resizeSurface(AppCtx *ctx, int dx, int dy, int width, int height)
{
    // How?
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

static GLuint createShader(AppCtx * ctx, GLenum shaderType, const char * shaderSource)
{
    GLuint shader= 0;
    GLint shaderStatus;
    GLsizei length;
    char logText[1000];

    shader= glCreateShader( shaderType );
    if ( shader )
    {
        glShaderSource(shader, 1, (const char **)&shaderSource, nullptr );
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderStatus);
        if (!shaderStatus)
        {
            glGetShaderInfoLog(shader, sizeof(logText), &length, logText);
            cerr << "Error compiling " << ((shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment") << " shader: "
                 << string(logText, length) << endl;
        }
    }

    return shader;
}

static bool setupGL(AppCtx *ctx)
{
    bool result= false;
    GLuint frag, vert;
    GLuint program;
    GLint status;

    frag= createShader(ctx, GL_FRAGMENT_SHADER, frag_shader_text);
    vert= createShader(ctx, GL_VERTEX_SHADER, vert_shader_text);

    program= glCreateProgram();
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(program, 1000, &len, log);
        cerr << "Error: linking:" << endl << string(log, len) << endl;
        goto exit;
    }

    glUseProgram(program);

    ctx->gl.pos= 0;
    ctx->gl.col= 1;

    glBindAttribLocation(program, ctx->gl.pos, "pos");
    glBindAttribLocation(program, ctx->gl.col, "color");
    glLinkProgram(program);

    ctx->gl.rotation_uniform= glGetUniformLocation(program, "rotation");

    exit:
    return result;
}

static bool renderGL(AppCtx *ctx)
{
    static const GLfloat verts[3][2] = {
    { -0.5, -0.5 },
    {  0.5, -0.5 },
    {  0,    0.5 }
    };
    static const GLfloat colors[3][4] = {
    { 1, 0, 0, 1.0 },
    { 0, 1, 0, 1.0 },
    { 0, 0, 1, 1.0 }
    };
    GLfloat angle;
    GLfloat rotation[4][4] = {
    { 1, 0, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 0, 1 }
    };
    static const uint32_t speed_div = 5;
    EGLint rect[4];

    glViewport(0, 0, ctx->planeWidth, ctx->planeHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ctx->currTime= currentTimeMillis();

    angle = ctx->noAnimation ? 0.0 : ((ctx->currTime-ctx->startTime) / speed_div) % 360 * M_PI / 180.0;
    rotation[0][0] =  cos(angle);
    rotation[0][2] =  sin(angle);
    rotation[2][0] = -sin(angle);
    rotation[2][2] =  cos(angle);

    glUniformMatrix4fv(ctx->gl.rotation_uniform, 1, GL_FALSE, (GLfloat *) rotation);

    glVertexAttribPointer(ctx->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
    glVertexAttribPointer(ctx->gl.col, 4, GL_FLOAT, GL_FALSE, 0, colors);
    glEnableVertexAttribArray(ctx->gl.pos);
    glEnableVertexAttribArray(ctx->gl.col);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(ctx->gl.pos);
    glDisableVertexAttribArray(ctx->gl.col);

    GLenum err= glGetError();
    if ( err != GL_NO_ERROR )
    {
        cerr << "renderGL: glGetError() = " << err << endl;
    }
}

