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


#include <dlfcn.h>
#include <EGL/egl.h>
#include "westeros-gl.h"


/*
 * WstGLNativePixmap:
 */
typedef struct _WstGLNativePixmap
{
    void *pixmap;
    int width;
    int height;
} WstNativePixmap;

typedef struct _WstGLNativeWindow
{
public:
    void swap() {}
private:
} WstGLNativeWindow;

typedef struct _WstGLCtx
{
    int refCnt;
    NativeDisplayType display;
    EGLDisplay dpy;
} WstGLCtx;

static WstGLCtx* g_wstCtx= NULL;

typedef EGLDisplay (*PREALEGLGETDISPLAY)(EGLNativeDisplayType);
static PREALEGLGETDISPLAY gRealEGLGetDisplay= 0;

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType displayId)
{
    EGLDisplay eglDisplay= EGL_NO_DISPLAY;

    printf("westeros-gl: eglGetDisplay: enter: displayId %x\n", displayId);

    if ( !g_wstCtx )
    {
        g_wstCtx= WstGLInit();
    }

    if ( !gRealEGLGetDisplay )
    {
        printf("westeros-gl: eglGetDisplay: failed linkage to underlying EGL impl\n" );
        goto exit;
    }

    if ( displayId == EGL_DEFAULT_DISPLAY )
    {
        if ( !g_wstCtx->display )
        {
            g_wstCtx->display= fbGetDisplay( g_wstCtx );
        }
        if ( g_wstCtx->display )
        {
            g_wstCtx->dpy = gRealEGLGetDisplay( (NativeDisplayType)g_wstCtx->display );
        }
    }
    else
    {
        g_wstCtx->dpy = gRealEGLGetDisplay(displayId);
    }

    if ( g_wstCtx->dpy )
    {
        eglDisplay= g_wstCtx->dpy;
    }

    exit:

    return eglDisplay;
}


WstGLCtx* WstGLInit()
{
    /*
     *  Establish the overloading of a subset of EGL methods
     */
    if ( !gRealEGLGetDisplay )
    {
        gRealEGLGetDisplay= (PREALEGLGETDISPLAY)dlsym( RTLD_NEXT, "eglGetDisplay" );
        printf("westeros-gl: wstGLInit: realEGLGetDisplay=%p\n", (void*)gRealEGLGetDisplay );
        if ( !gRealEGLGetDisplay )
        {
            printf("westeros-gl: wstGLInit: unable to resolve eglGetDisplay\n");
            goto exit;
        }
    }

    if( g_wstCtx != NULL )
    {
        ++g_wstCtx->refCnt;
        return g_wstCtx;
    }

    g_wstCtx= (WstGLCtx*)calloc(1, sizeof(WstGLCtx));
    if ( g_wstCtx )
    {
        g_wstCtx->refCnt= 1;
    }

    exit:

    return g_wstCtx;
}

void WstGLTerm( WstGLCtx *ctx )
{
    if ( ctx )
    {
        if ( ctx != g_wstCtx )
        {
            printf("westeros-gl: WstGLTerm: bad ctx %p, should be %p\n", ctx, g_wstCtx );
            return;
        }

        --ctx->refCnt;
        if ( ctx->refCnt <= 0 )
        {
            if ( ctx->display )
            {
                fbDestroyDisplay(ctx->display);
                ctx->display= NULL;
            }

            free( ctx );

            g_wstCtx= 0;
        }
    }
}

/*
 * WstGLCreateNativeWindow
 * Create a native window suitable for use as an EGLNativeWindow
 */
void* WstGLCreateNativeWindow( WstGLCtx *ctx, int x, int y, int width, int height )
{
    void *nativeWindow= 0;
    if ( ctx ) {
        nativeWindow = fbCreateWindow(ctx->display, x, y, width, height);
    }
    return nativeWindow;
}

/*
 * WstGLDestroyNativeWindow
 * Destroy a native window created by WstGLCreateNativeWindow
 */
void WstGLDestroyNativeWindow( WstGLCtx *ctx, void *nativeWindow )
{
    if ( ctx && nativeWindow) {
        fbDestroyWindow(nativeWindow);
    }
}

bool WstGLGetNativePixmap( WstGLCtx *ctx, void *nativeBuffer, void **nativePixmap )
{
    bool result= false;

    if ( ctx )
    {
        // Not yet required
    }

    return result;
}

/*
 * WstGLGetNativePixmapDimensions
 * Get the dimensions of the WstGLNativePixmap
 */
void WstGLGetNativePixmapDimensions( WstGLCtx *ctx, void *nativePixmap, int *width, int *height )
{
    if ( ctx )
    {
        // Not yet required
    }
}

/*
 * WstGLReleaseNativePixmap
 * Release a WstGLNativePixmap obtained via WstGLGetNativePixmap
 */
void WstGLReleaseNativePixmap( WstGLCtx *ctx, void *nativePixmap )
{
    if ( ctx )
    {
        // Not yet required
    }
}

/*
 * WstGLGetEGLNativePixmap
 * Get the native pixmap usable as a EGL_NATIVE_PIXMAP_KHR for creating a texture
 * from the provided WstGLNativePixmap instance
 */
EGLNativePixmapType WstGLGetEGLNativePixmap( WstGLCtx *ctx, void *nativePixmap )
{
    EGLNativePixmapType eglPixmap= 0;

    if ( nativePixmap )
    {
        // Not yet required
    }

    return eglPixmap;
}

