/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
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

#include "essos.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <linux/input.h>
#include <poll.h>
#include <pthread.h>
#include <sys/time.h>

#include <vector>

#ifdef HAVE_WAYLAND
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include "wayland-client.h"
#include "wayland-egl.h"
#endif

#ifdef HAVE_WESTEROS
#include "westeros-gl.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define ESS_UNUSED(x) ((void)x)
#define ESS_MAX_ERROR_DETAIL (512)
#define DEFAULT_PLANE_WIDTH (1280)
#define DEFAULT_PLANE_HEIGHT (720)

#define INT_FATAL(FORMAT, ...)      printf("Essos Fatal: " FORMAT "\n", ##__VA_ARGS__)
#define INT_ERROR(FORMAT, ...)      printf("Essos Error: " FORMAT "\n", ##__VA_ARGS__)
#define INT_WARNING(FORMAT, ...)    printf("Essos Warning: " FORMAT "\n",  ##__VA_ARGS__)
#define INT_INFO(FORMAT, ...)       printf("Essos Info: " FORMAT "\n",  ##__VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)
#define INT_TRACE(FORMAT, ...)

#define FATAL(FORMAT, ...)          INT_FATAL(FORMAT, ##__VA_ARGS__)
#define ERROR(FORMAT, ...)          INT_ERROR(FORMAT, ##__VA_ARGS__)
#define WARNING(FORMAT, ...)        INT_WARNING(FORMAT, ##__VA_ARGS__)
#define INFO(FORMAT, ...)           INT_INFO(FORMAT, ##__VA_ARGS__)
#define DEBUG(FORMAT, ...)          INT_DEBUG(FORMAT, ##__VA_ARGS__)
#define TRACE(FORMAT, ...)          INT_TRACE(FORMAT, ##__VA_ARGS__)

typedef struct _EssCtx
{
   pthread_mutex_t mutex;
   bool isWayland;
   bool isInitialized;
   bool isRunning;
   char lastErrorDetail[ESS_MAX_ERROR_DETAIL];

   int planeWidth;
   int planeHeight;
   int waylandFd;
   pollfd wlPollFd;
   int notifyFd;
   int watchFd;
   std::vector<pollfd> inputDeviceFds;
   int eventLoopPeriodMS;

   int pointerX;
   int pointerY;

   void *keyListenerUserData;
   EssKeyListener *keyListener;
   void *pointerListenerUserData;
   EssPointerListener *pointerListener;
   void *terminateListenerUserData;
   EssTerminateListener *terminateListener;

   NativeDisplayType displayType;
   NativeWindowType nativeWindow;
   EGLDisplay eglDisplay;
   EGLint eglVersionMajor;
   EGLint eglVersionMinor;
   EGLint *eglCfgAttr;
   EGLint eglCfgAttrSize;
   EGLConfig eglConfig;
   EGLint *eglCtxAttr;
   EGLint eglCtxAttrSize;
   EGLContext eglContext;
   EGLSurface eglSurfaceWindow;
   EGLint eglSwapInterval;

   #ifdef HAVE_WAYLAND
   struct wl_display *wldisplay;
   struct wl_registry *wlregistry;
   struct wl_compositor *wlcompositor;
   struct wl_seat *wlseat;
   struct wl_output *wloutput;
   struct wl_keyboard *wlkeyboard;
   struct wl_pointer *wlpointer;
   struct wl_touch *wltouch;
   struct wl_surface *wlsurface;
   struct wl_egl_window *wleglwindow;

   struct xkb_context *xkbCtx;
   struct xkb_keymap *xkbKeymap;
   struct xkb_state *xkbState;
   xkb_mod_index_t modAlt;
   xkb_mod_index_t modCtrl;
   xkb_mod_index_t modShift;
   xkb_mod_index_t modCaps;
   unsigned int modMask;
   #endif
   #ifdef HAVE_WESTEROS
   WstGLCtx *glCtx;
   #endif
} EssCtx;

static long long essGetCurrentTimeMillis(void);
static bool essPlatformInit( EssCtx *ctx );
static void essPlatformTerm( EssCtx *ctx );
static bool essEGLInit( EssCtx *ctx );
static void essEGLTerm( EssCtx *ctx );
static void essInitInput( EssCtx *ctx );
static bool essCreateNativeWindow( EssCtx *ctx, int width, int height );
static void essRunEventLoopOnce( EssCtx *ctx );
static void essProcessKeyPressed( EssCtx *ctx, int linuxKeyCode );
static void essProcessKeyReleased( EssCtx *ctx, int linuxKeyCode );
static void essProcessPointerMotion( EssCtx *ctx, int x, int y );
static void essProcessPointerButtonPressed( EssCtx *ctx, int button );
static void essProcessPointerButtonReleased( EssCtx *ctx, int button );
#ifdef HAVE_WAYLAND
static bool essPlatformInitWayland( EssCtx *ctx );
static void essPlatformTermWayland( EssCtx *ctx );
static void essProcessRunWaylandEventLoopOnce( EssCtx *ctx );
#endif
#ifdef HAVE_WESTEROS
static bool essPlatformInitDirect( EssCtx *ctx );
static void essPlatformTermDirect( EssCtx *ctx );
static int essOpenInputDevice( EssCtx *ctx, const char *devPathName );
static char *essGetInputDevice( EssCtx *ctx, const char *path, char *devName );
static void essGetInputDevices( EssCtx *ctx );
static void essMonitorInputDevicesLifecycleBegin( EssCtx *ctx );
static void essMonitorInputDevicesLifecycleEnd( EssCtx *ctx );
static void essReleaseInputDevices( EssCtx *ctx );
static void essProcessInputDevices( EssCtx *ctx );
#endif

static EGLint gDefaultEGLCfgAttrSize= 17;
static EGLint gDefaultEGLCfgAttr[]=
{
  EGL_RED_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_BLUE_SIZE, 8,
  EGL_ALPHA_SIZE, 8,
  EGL_DEPTH_SIZE, 0,
  EGL_STENCIL_SIZE, 0,
  EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
  EGL_NONE
};

static EGLint gDefaultEGLCtxAttrSize= 3;
static EGLint gDefaultEGLCtxAttr[]=
{
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE
};

EssCtx* EssContextCreate()
{
   EssCtx *ctx= 0;

   ctx= (EssCtx*)calloc( 1, sizeof(EssCtx) );
   if ( ctx )
   {
      pthread_mutex_init( &ctx->mutex, 0 );

      ctx->planeWidth= DEFAULT_PLANE_WIDTH;
      ctx->planeHeight= DEFAULT_PLANE_HEIGHT;
      ctx->notifyFd= -1;
      ctx->watchFd= -1;
      ctx->waylandFd= -1;
      ctx->eventLoopPeriodMS= 16;

      ctx->displayType= EGL_DEFAULT_DISPLAY;
      ctx->eglDisplay= EGL_NO_DISPLAY;
      ctx->eglCfgAttr= gDefaultEGLCfgAttr;
      ctx->eglCfgAttrSize= gDefaultEGLCfgAttrSize;
      ctx->eglCtxAttr= gDefaultEGLCtxAttr;
      ctx->eglCtxAttrSize= gDefaultEGLCtxAttrSize;
      ctx->eglContext= EGL_NO_CONTEXT;
      ctx->eglSurfaceWindow= EGL_NO_SURFACE;
      ctx->eglSwapInterval= 1;

      ctx->inputDeviceFds= std::vector<pollfd>();
   }

   return ctx;
}

void EssContextDestroy( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->isRunning )
      {
         EssContextStop( ctx );
      }

      essPlatformTerm( ctx );

      pthread_mutex_destroy( &ctx->mutex );
      
      free( ctx );
   }
}

const char *EssContextGetLastErrorDetail( EssCtx *ctx )
{
   const char *detail= 0;
   
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );
      
      detail= ctx->lastErrorDetail;
      
      pthread_mutex_unlock( &ctx->mutex );
   }
   
   return detail;
}

bool EssContextSupportWayland( EssCtx *ctx )
{
   #ifdef HAVE_WAYLAND
   return true;
   #else
   return false;
   #endif
}

bool EssContextSupportDirect( EssCtx *ctx )
{
   #ifdef HAVE_WESTEROS
   // We use westeros-gl to hide SOC specifics of EGL
   return true;
   #else
   return false;
   #endif
}

bool EssContextSetUseWayland( EssCtx *ctx, bool useWayland )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( ctx->isInitialized )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Can't change application type when already initialized" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      #ifndef HAVE_WAYLAND
      if ( useWayland )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Wayland mode is not available" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }
      #endif

      #ifndef HAVE_WESTEROS
      if ( !useWayland )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Direct mode is not available" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }
      #endif

      ctx->isWayland= useWayland;

      pthread_mutex_unlock( &ctx->mutex );

      result= true;
   }

exit:   

   return result;
}

bool EssContextGetUseWayland( EssCtx *ctx )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      result= ctx->isWayland;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:   

   return result;      
}

bool EssContextSetUseDirect( EssCtx *ctx, bool useWayland )
{
   return EssContextSetUseWayland( ctx, !useWayland );
}

bool EssContextGetUseDirect( EssCtx *ctx )
{
   return !EssContextGetUseWayland( ctx );
}

bool EssContextSetEGLSurfaceAttributes( EssCtx *ctx, EGLint *attrs, EGLint size )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      ctx->eglCfgAttr= (attrs ? attrs : gDefaultEGLCfgAttr);
      ctx->eglCfgAttrSize= (attrs ? size : gDefaultEGLCfgAttrSize);

      pthread_mutex_unlock( &ctx->mutex );

      result= true;
   }

   return result;
}

bool EssContextGetEGLSurfaceAttributes( EssCtx *ctx, EGLint **attrs, EGLint *size )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( attrs && size )
      {
         *attrs= ctx->eglCfgAttr;
         *size= ctx->eglCfgAttrSize;
         result= true;
      }

      pthread_mutex_unlock( &ctx->mutex );
   }

   return result;
}


bool EssContextSetEGLContextAttributes( EssCtx *ctx, EGLint *attrs, EGLint size )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      ctx->eglCtxAttr= (attrs ? attrs : gDefaultEGLCtxAttr);
      ctx->eglCtxAttrSize= (attrs ? size : gDefaultEGLCtxAttrSize);

      pthread_mutex_unlock( &ctx->mutex );

      result= true;
   }

   return result;
}

bool EssContextGetEGLContextAttributes( EssCtx *ctx, EGLint **attrs, EGLint *size )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( attrs && size )
      {
         *attrs= ctx->eglCtxAttr;
         *size= ctx->eglCtxAttrSize;
         result= true;
      }

      pthread_mutex_unlock( &ctx->mutex );
   }

   return result;
}

bool EssContextSetInitialWindowSize( EssCtx *ctx, int width, int height )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );
      
      if ( ctx->isRunning )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Context is already running" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      ctx->planeWidth= width;
      ctx->planeHeight= height;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:   

   return result;
}

bool EssContextSetSwapInterval( EssCtx *ctx, EGLint swapInterval )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );
      
      if ( ctx->isRunning )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Context is already running" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      ctx->eglSwapInterval= swapInterval;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:   

   return result;
}

bool EssContextInit( EssCtx *ctx )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );
      
      if ( ctx->isRunning )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Context is already running" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      pthread_mutex_unlock( &ctx->mutex );

      if ( !EssContextSetUseWayland( ctx, ctx->isWayland ) )
      {
         goto exit;
      }

      pthread_mutex_lock( &ctx->mutex );

      result= essPlatformInit(ctx);
      if ( result )
      {
         ctx->isInitialized= true;
      }

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:   

   return result;
}

bool EssContextGetEGLDisplayType( EssCtx *ctx, NativeDisplayType *displayType )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( !ctx->isInitialized )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Must initialize before getting display type" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      *displayType= ctx->displayType;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:
   return result;
}

bool EssContextCreateNativeWindow( EssCtx *ctx, int width, int height, NativeWindowType *nativeWindow )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( !ctx->isInitialized )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Must initialize before creating native window" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      result= essCreateNativeWindow( ctx, width, height );
      if ( result )
      {
         *nativeWindow= ctx->nativeWindow;
      }

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:
   return result;
}

void* EssContextGetWaylandDisplay( EssCtx *ctx )
{
   void *wldisplay= 0;

   #ifdef HAVE_WAYLAND
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      wldisplay= (void*)ctx->wldisplay;   

      pthread_mutex_unlock( &ctx->mutex );
   }
   #endif

   return wldisplay;
}

bool EssContextSetKeyListener( EssCtx *ctx, void *userData, EssKeyListener *listener )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      ctx->keyListenerUserData= userData;
      ctx->keyListener= listener;

      result= true;

      pthread_mutex_unlock( &ctx->mutex );
   }

   return result;
}


bool EssContextSetPointerListener( EssCtx *ctx, void *userData, EssPointerListener *listener )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      ctx->pointerListenerUserData= userData;
      ctx->pointerListener= listener;

      result= true;

      pthread_mutex_unlock( &ctx->mutex );
   }

   return result;
}

bool EssContextSetTerminateListener( EssCtx *ctx, void *userData, EssTerminateListener *listener )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      ctx->terminateListenerUserData= userData;
      ctx->terminateListener= listener;

      result= true;

      pthread_mutex_unlock( &ctx->mutex );
   }

   return result;
}

bool EssContextStart( EssCtx *ctx )
{
   bool result= false;
                  
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );
      
      if ( ctx->isRunning )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Context is already running" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      if ( !ctx->isInitialized )
      {
         pthread_mutex_unlock( &ctx->mutex );
         result= EssContextInit( ctx );
         if ( !result ) goto exit;
         pthread_mutex_lock( &ctx->mutex );
      }

      result= essEGLInit( ctx );
      if ( !result )
      {
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      essInitInput( ctx );

      ctx->isRunning= true;

      essRunEventLoopOnce( ctx );

      result= true;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:   

   return result;
}

void EssContextStop( EssCtx *ctx )
{
   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( ctx->isRunning )
      {
         if  (!ctx->isWayland )
         {
            essMonitorInputDevicesLifecycleBegin( ctx );
            essReleaseInputDevices( ctx );
         }

         essEGLTerm( ctx );

         ctx->isRunning= false;
      }

      if ( ctx->isInitialized )
      {
         essPlatformTerm( ctx );
      }

      pthread_mutex_unlock( &ctx->mutex );
   }
}

bool EssContextGetDisplaySize( EssCtx *ctx, int *width, int *height )
{
   bool result= false;

   if ( ctx )
   {
      pthread_mutex_lock( &ctx->mutex );

      if ( !ctx->isInitialized )
      {
         sprintf( ctx->lastErrorDetail,
                  "Bad state.  Must initialize before querying display size" );
         pthread_mutex_unlock( &ctx->mutex );
         goto exit;
      }

      if ( width )
      {
         *width= ctx->planeWidth;
      }
      if ( height )
      {
         *height= ctx->planeHeight;
      }

      result= true;

      pthread_mutex_unlock( &ctx->mutex );
   }

exit:

   return result;
}

void EssContextRunEventLoopOnce( EssCtx *ctx )
{
   if ( ctx )
   {
      long long start, end, diff, delay;
      
      if ( ctx->eventLoopPeriodMS )
      {
         start= essGetCurrentTimeMillis();
      }

      essRunEventLoopOnce( ctx );

      if ( ctx->eventLoopPeriodMS )
      {
         end= essGetCurrentTimeMillis();
         diff= end-start;
         delay= ((long long)ctx->eventLoopPeriodMS - diff);
         if ( delay > 0 )
         {
            usleep( delay*1000 );
         }
      }
   }
}

void EssContextUpdateDisplay( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( (ctx->eglDisplay != EGL_NO_DISPLAY) &&
           (ctx->eglSurfaceWindow != EGL_NO_SURFACE) )
      {
         eglSwapBuffers( ctx->eglDisplay, ctx->eglSurfaceWindow );
      }
   }
}

static long long essGetCurrentTimeMillis(void)
{
   struct timeval tv;
   long long utcCurrentTimeMillis;

   gettimeofday(&tv,0);
   utcCurrentTimeMillis= tv.tv_sec*1000LL+(tv.tv_usec/1000LL);

   return utcCurrentTimeMillis;
}

static bool essPlatformInit( EssCtx *ctx )
{
   bool result= false;

   if ( ctx->isWayland )
   {
      #ifdef HAVE_WAYLAND
      result= essPlatformInitWayland( ctx );
      #endif
   }
   else
   {
      #ifdef HAVE_WESTEROS
      result= essPlatformInitDirect( ctx );
      #endif
   }

   return result;   
}

static void essPlatformTerm( EssCtx *ctx )
{
   if ( ctx->isWayland )
   {
      #ifdef HAVE_WAYLAND
      essPlatformTermWayland( ctx );
      #endif
   }
   else
   {
      #ifdef HAVE_WESTEROS
      essPlatformTermDirect( ctx );
      #endif
   }
}

static bool essEGLInit( EssCtx *ctx )
{
   bool result= false;
   EGLBoolean b;
   EGLConfig *eglConfigs= 0;
   EGLint configCount= 0;
   EGLint redSizeNeed, greenSizeNeed, blueSizeNeed, alphaSizeNeed, depthSizeNeed;
   EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
   int i;
   
   DEBUG("essEGLInit: displayType %p\n", ctx->displayType);
   ctx->eglDisplay= eglGetDisplay( ctx->displayType );
   if ( ctx->eglDisplay == EGL_NO_DISPLAY )
   {
      sprintf( ctx->lastErrorDetail,
               "Error. Unable to get EGL display: eglError %X", eglGetError() );
      goto exit;
   }

   b= eglInitialize( ctx->eglDisplay, &ctx->eglVersionMajor, &ctx->eglVersionMinor );
   if ( !b )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: Unable to initialize EGL display: eglError %X", eglGetError() );
      goto exit;
   }

   b= eglChooseConfig( ctx->eglDisplay, ctx->eglCfgAttr, 0, 0, &configCount );
   if ( !b || (configCount == 0) )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: eglChooseConfig failed to return number of configurations: count: %d eglError %X\n", configCount, eglGetError() );
      goto exit;
   }

   eglConfigs= (EGLConfig*)malloc( configCount*sizeof(EGLConfig) );
   if ( !eglConfigs )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: Unable to allocate memory for %d EGL configurations", configCount );
      goto exit;
   }

   b= eglChooseConfig( ctx->eglDisplay, ctx->eglCfgAttr, eglConfigs, configCount, &configCount );
   if ( !b || (configCount == 0) )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: eglChooseConfig failed to return list of configurations: count: %d eglError %X\n", configCount, eglGetError() );
      goto exit;
   }

   redSizeNeed= greenSizeNeed= blueSizeNeed= alphaSizeNeed= depthSizeNeed= 0;
   for( i= 0; i < ctx->eglCfgAttrSize; i += 2 )
   {
      EGLint type= ctx->eglCfgAttr[i];
      if ( (type != EGL_NONE) && (i+1 < ctx->eglCfgAttrSize) )
      {
         EGLint value= ctx->eglCfgAttr[i+1];
         switch( ctx->eglCfgAttr[i] )
         {
            case EGL_RED_SIZE:
               redSizeNeed= value;
               break;
            case EGL_GREEN_SIZE:
               greenSizeNeed= value;
               break;
            case EGL_BLUE_SIZE:
               blueSizeNeed= value;
               break;
            case EGL_ALPHA_SIZE:
               alphaSizeNeed= value;
               break;
            case EGL_DEPTH_SIZE:
               depthSizeNeed= value;
               break;
            default:
               break;
         }
      }
   }

   for( i= 0; i < configCount; ++i )
   {
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_RED_SIZE, &redSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_GREEN_SIZE, &greenSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_BLUE_SIZE, &blueSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_ALPHA_SIZE, &alphaSize );
      eglGetConfigAttrib( ctx->eglDisplay, eglConfigs[i], EGL_DEPTH_SIZE, &depthSize );

      DEBUG("essEGLInit: config %d: red: %d green: %d blue: %d alpha: %d depth: %d\n",
              i, redSize, greenSize, blueSize, alphaSize, depthSize );
      if ( (redSize == redSizeNeed) &&
           (greenSize == greenSizeNeed) &&
           (blueSize == blueSizeNeed) &&
           (alphaSize == alphaSizeNeed) &&
           (depthSize >= depthSizeNeed) )
      {
         DEBUG( "essEGLInit: choosing config %d\n", i);
         break;
      }
   }

   if ( i == configCount )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: no suitable configuration available\n");
      goto exit;
   }

   ctx->eglConfig= eglConfigs[i];

   ctx->eglContext= eglCreateContext( ctx->eglDisplay, ctx->eglConfig, EGL_NO_CONTEXT, ctx->eglCtxAttr );
   if ( ctx->eglContext == EGL_NO_CONTEXT )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: eglCreateContext failed: eglError %X\n", eglGetError() );
      goto exit;
   }
   DEBUG("essEGLInit: eglContext %p\n", ctx->eglContext );

   if ( !essCreateNativeWindow( ctx, ctx->planeWidth, ctx->planeHeight ) )
   {
      goto exit;
   }
   DEBUG("essEGLInit: nativeWindow %p\n", ctx->nativeWindow );

   ctx->eglSurfaceWindow= eglCreateWindowSurface( ctx->eglDisplay,
                                                  ctx->eglConfig,
                                                  ctx->nativeWindow,
                                                  NULL );
   if ( ctx->eglSurfaceWindow == EGL_NO_SURFACE )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: eglCreateWindowSurface failed: eglError %X\n", eglGetError() );
      goto exit;
   }
   DEBUG("essEGLInit: eglSurfaceWindow %p\n", ctx->eglSurfaceWindow );

   b= eglMakeCurrent( ctx->eglDisplay, ctx->eglSurfaceWindow, ctx->eglSurfaceWindow, ctx->eglContext );
   if ( !b )
   {
      sprintf( ctx->lastErrorDetail,
               "Error: eglMakeCurrent failed: eglError %X\n", eglGetError() );
      goto exit;
   }
    
   eglSwapInterval( ctx->eglDisplay, ctx->eglSwapInterval );

   result= true;

exit:
   if ( !result )
   {
      if ( eglConfigs )
      {
         free( eglConfigs );
      }
      essEGLTerm(ctx);
   }

   return result;
}

static void essEGLTerm( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->eglDisplay != EGL_NO_DISPLAY )
      {
         eglMakeCurrent( ctx->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );

         if ( ctx->eglSurfaceWindow != EGL_NO_SURFACE )
         {
            eglDestroySurface( ctx->eglDisplay, ctx->eglSurfaceWindow );
            ctx->eglSurfaceWindow= EGL_NO_SURFACE;
         }

         if ( ctx->eglContext != EGL_NO_CONTEXT )
         {
            eglDestroyContext( ctx->eglDisplay, ctx->eglContext );
            ctx->eglContext= EGL_NO_CONTEXT;
         }
         
         eglTerminate( ctx->eglDisplay );
         ctx->eglDisplay= EGL_NO_DISPLAY;

         eglReleaseThread();
      }
   }
}

static void essInitInput( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->isWayland )
      {
         // Setup during wayland registry processing
      }
      else
      {
         essGetInputDevices( ctx );
         essMonitorInputDevicesLifecycleBegin( ctx );
      }
   }
}

static bool essCreateNativeWindow( EssCtx *ctx, int width, int height )
{
   bool result= false;

   if ( ctx )
   {
      if ( ctx->isWayland )
      {
         #ifdef HAVE_WAYLAND
         ctx->wlsurface= wl_compositor_create_surface(ctx->wlcompositor);
         if ( !ctx->wlsurface )
         {
            sprintf( ctx->lastErrorDetail,
                     "Error.  Unable to create wayland surface" );
            pthread_mutex_unlock( &ctx->mutex );
            goto exit;
         }

         ctx->wleglwindow= wl_egl_window_create(ctx->wlsurface, width, height);
         if ( !ctx->wleglwindow )
         {
            sprintf( ctx->lastErrorDetail,
                     "Error.  Unable to create wayland egl window" );
            pthread_mutex_unlock( &ctx->mutex );
            goto exit;
         }

         ctx->nativeWindow= (NativeWindowType)ctx->wleglwindow;         

         result= true;
         #endif
      }
      else
      {
         #ifdef HAVE_WESTEROS
         ctx->nativeWindow= (NativeWindowType)WstGLCreateNativeWindow( ctx->glCtx, 
                                                                       0,
                                                                       0,
                                                                       width,
                                                                       height );
         if ( !ctx->nativeWindow )
         {
            sprintf( ctx->lastErrorDetail,
                     "Error.  Unable to create native egl window" );
            pthread_mutex_unlock( &ctx->mutex );
            goto exit;
         }

         result= true;
         #endif
      }
   }

exit:
   return result;
}

static void essRunEventLoopOnce( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->isWayland )
      {
         #ifdef HAVE_WAYLAND
         essProcessRunWaylandEventLoopOnce( ctx );
         #endif
      }
      else
      {
         #ifdef HAVE_WESTEROS
         essProcessInputDevices( ctx );
         #endif
      }
   }
}

static void essProcessKeyPressed( EssCtx *ctx, int linuxKeyCode )
{
   if ( ctx )
   {
      DEBUG("essProcessKeyPressed: key %d\n", linuxKeyCode);
      if ( ctx->keyListener && ctx->keyListener->keyPressed )
      {
         ctx->keyListener->keyPressed( ctx->keyListenerUserData, linuxKeyCode );
      }
   }
}

static void essProcessKeyReleased( EssCtx *ctx, int linuxKeyCode )
{
   if ( ctx )
   {
      DEBUG("essProcessKeyReleased: key %d\n", linuxKeyCode);
      if ( ctx->keyListener && ctx->keyListener->keyReleased )
      {
         ctx->keyListener->keyReleased( ctx->keyListenerUserData, linuxKeyCode );
      }
   }
}

static void essProcessPointerMotion( EssCtx *ctx, int x, int y )
{
   if ( ctx )
   {
      TRACE("essProcessKeyPointerMotion (%d, %d)\n", x, y );
      ctx->pointerX= x;
      ctx->pointerY= y;
      if ( ctx->pointerListener && ctx->pointerListener->pointerMotion )
      {
         ctx->pointerListener->pointerMotion( ctx->pointerListenerUserData, x,  y );
      }
   }
}

static void essProcessPointerButtonPressed( EssCtx *ctx, int button )
{
   if ( ctx )
   {
      DEBUG("essProcessKeyPointerPressed %d\n", button );
      if ( ctx->pointerListener && ctx->pointerListener->pointerButtonPressed )
      {
         ctx->pointerListener->pointerButtonPressed( ctx->pointerListenerUserData, button, ctx->pointerX, ctx->pointerY );
      }
   }
}

static void essProcessPointerButtonReleased( EssCtx *ctx, int button )
{
   if ( ctx )
   {
      DEBUG("essos: essProcessKeyPointerReleased %d\n", button );
      if ( ctx->pointerListener && ctx->pointerListener->pointerButtonReleased )
      {
         ctx->pointerListener->pointerButtonReleased( ctx->pointerListenerUserData, button, ctx->pointerX, ctx->pointerY );
      }
   }
}

#ifdef HAVE_WAYLAND
static void essKeyboardKeymap( void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size )
{
   EssCtx *ctx= (EssCtx*)data;

   if ( format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 )
   {
      void *map= mmap( 0, size, PROT_READ, MAP_SHARED, fd, 0 );
      if ( map != MAP_FAILED )
      {
         if ( !ctx->xkbCtx )
         {
            ctx->xkbCtx= xkb_context_new( XKB_CONTEXT_NO_FLAGS );
         }
         else
         {
            ERROR("essKeyboardKeymap: xkb_context_new failed\n");
         }
         if ( ctx->xkbCtx )
         {
            if ( ctx->xkbKeymap )
            {
               xkb_keymap_unref( ctx->xkbKeymap );
               ctx->xkbKeymap= 0;
            }
            ctx->xkbKeymap= xkb_keymap_new_from_string( ctx->xkbCtx, (char*)map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
            if ( !ctx->xkbKeymap )
            {
               ERROR("essKeyboardKeymap: xkb_keymap_new_from_string failed\n");
            }
            if ( ctx->xkbState )
            {
               xkb_state_unref( ctx->xkbState );
               ctx->xkbState= 0;
            }
            ctx->xkbState= xkb_state_new( ctx->xkbKeymap );
            if ( !ctx->xkbState )
            {
               ERROR("essKeyboardKeymap: xkb_state_new failed\n");
            }
            if ( ctx->xkbKeymap )
            {
               ctx->modAlt= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_ALT );
               ctx->modCtrl= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_CTRL );
               ctx->modShift= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_SHIFT );
               ctx->modCaps= xkb_keymap_mod_get_index( ctx->xkbKeymap, XKB_MOD_NAME_CAPS );
            }
            munmap( map, size );
         }
      }
   }

   close( fd );
}

static void essKeyboardEnter( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                              struct wl_surface *surface, struct wl_array *keys )
{
   ESS_UNUSED(data);
   ESS_UNUSED(keyboard);
   ESS_UNUSED(serial);
   ESS_UNUSED(keys);

   DEBUG("essKeyboardEnter: keyboard enter surface %p\n", surface );
}

static void essKeyboardLeave( void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface )
{
   ESS_UNUSED(data);
   ESS_UNUSED(keyboard);
   ESS_UNUSED(serial);

   DEBUG("esKeyboardLeave: keyboard leave surface %p\n", surface );
}

static void essKeyboardKey( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                            uint32_t time, uint32_t key, uint32_t state )
{
   EssCtx *ctx= (EssCtx*)data;
   ESS_UNUSED(keyboard);
   ESS_UNUSED(serial);
   ESS_UNUSED(time);

   switch( key )
   {
      case KEY_CAPSLOCK:
      case KEY_LEFTSHIFT:
      case KEY_RIGHTSHIFT:
      case KEY_LEFTCTRL:
      case KEY_RIGHTCTRL:
      case KEY_LEFTALT:
      case KEY_RIGHTALT:
         break;
      default:
         if ( state == WL_KEYBOARD_KEY_STATE_PRESSED )
         {
            essProcessKeyPressed( ctx, key );
         }
         else if ( state == WL_KEYBOARD_KEY_STATE_RELEASED )
         {
            essProcessKeyReleased( ctx, key );
         }
         break;
   }
}

static void essUpdateModifierKey( EssCtx *ctx, bool active, int key )
{
   if ( active )
   {
      essProcessKeyPressed( ctx, key );
   }
   else
   {
      essProcessKeyReleased( ctx, key );
   }
}

static void essKeyboardModifiers( void *data, struct wl_keyboard *keyboard, uint32_t serial,
                                  uint32_t mods_depressed, uint32_t mods_latched,
                                  uint32_t mods_locked, uint32_t group )
{
   EssCtx *ctx= (EssCtx*)data;
   if ( ctx->xkbState )
   {
      int wasActive, nowActive, key;

      xkb_state_update_mask( ctx->xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group );
      DEBUG("essKeyboardModifiers: mods_depressed %X mods locked %X\n", mods_depressed, mods_locked);

      wasActive= (ctx->modMask & (1<<ctx->modCaps));
      nowActive= (mods_locked & (1<<ctx->modCaps));
      key= KEY_CAPSLOCK;
      if ( nowActive != wasActive )
      {
         ctx->modMask ^= (1<<ctx->modCaps);
         essProcessKeyPressed( ctx, key );
         essProcessKeyReleased( ctx, key );
      }

      wasActive= (ctx->modMask & (1<<ctx->modShift));
      nowActive= (mods_depressed & (1<<ctx->modShift));
      key= KEY_RIGHTSHIFT;
      if ( nowActive != wasActive )
      {
         ctx->modMask ^= (1<<ctx->modShift);
         essUpdateModifierKey( ctx, nowActive, key );
      }

      wasActive= (ctx->modMask & (1<<ctx->modCtrl));
      nowActive= (mods_depressed & (1<<ctx->modCtrl));
      key= KEY_RIGHTCTRL;
      if ( nowActive != wasActive )
      {
         ctx->modMask ^= (1<<ctx->modCtrl);
         essUpdateModifierKey( ctx, nowActive, key );
      }

      wasActive= (ctx->modMask & (1<<ctx->modAlt));
      nowActive= (mods_depressed & (1<<ctx->modAlt));
      key= KEY_RIGHTALT;
      if ( nowActive != wasActive )
      {
         ctx->modMask ^= (1<<ctx->modAlt);
         essUpdateModifierKey( ctx, nowActive, key );
      }
   }
}

static void essKeyboardRepeatInfo( void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay )
{
   ESS_UNUSED(data);
   ESS_UNUSED(keyboard);
   ESS_UNUSED(rate);
   ESS_UNUSED(delay);
}

static const struct wl_keyboard_listener essKeyboardListener= {
   essKeyboardKeymap,
   essKeyboardEnter,
   essKeyboardLeave,
   essKeyboardKey,
   essKeyboardModifiers,
   essKeyboardRepeatInfo
};

static void essPointerEnter( void* data, struct wl_pointer *pointer, uint32_t serial,
                             struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy )
{
   ESS_UNUSED(pointer);
   ESS_UNUSED(serial);
   EssCtx *ctx= (EssCtx*)data;
   int x, y;

   x= wl_fixed_to_int( sx );
   y= wl_fixed_to_int( sy );

   DEBUG("essPointerEnter: pointer enter surface %p (%d,%d)\n", surface, x, y );
}

static void essPointerLeave( void* data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface )
{
   ESS_UNUSED(data);
   ESS_UNUSED(pointer);
   ESS_UNUSED(serial);

   DEBUG("essPointerLeave: pointer leave surface %p\n", surface );
}

static void essPointerMotion( void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy )
{
   ESS_UNUSED(pointer);
   EssCtx *ctx= (EssCtx*)data;
   int x, y;

   x= wl_fixed_to_int( sx );
   y= wl_fixed_to_int( sy );

   essProcessPointerMotion( ctx, x, y );
}

static void essPointerButton( void *data, struct wl_pointer *pointer, uint32_t serial,
                              uint32_t time, uint32_t button, uint32_t state )
{
   ESS_UNUSED(pointer);
   ESS_UNUSED(serial);
   EssCtx *ctx= (EssCtx*)data;

   if ( state == WL_POINTER_BUTTON_STATE_PRESSED )
   {
      essProcessPointerButtonPressed( ctx, button );
   }
   else
   {
      essProcessPointerButtonReleased( ctx, button );
   }
}

static void essPointerAxis( void *data, struct wl_pointer *pointer, uint32_t time,
                            uint32_t axis, wl_fixed_t value )
{
   ESS_UNUSED(data);
   ESS_UNUSED(pointer);
   ESS_UNUSED(time);
   ESS_UNUSED(value);
}

static const struct wl_pointer_listener essPointerListener = {
   essPointerEnter,
   essPointerLeave,
   essPointerMotion,
   essPointerButton,
   essPointerAxis
};

static void essSeatCapabilities( void *data, struct wl_seat *seat, uint32_t capabilities )
{
	EssCtx *ctx= (EssCtx*)data;

   DEBUG("essSeatCapabilities: seat %p caps: %X\n", seat, capabilities );
   
   if ( capabilities & WL_SEAT_CAPABILITY_KEYBOARD )
   {
      DEBUG("essSeatCapabilities:  seat has keyboard\n");
      ctx->wlkeyboard= wl_seat_get_keyboard( ctx->wlseat );
      DEBUG("essSeatCapabilities:  keyboard %p\n", ctx->wlkeyboard );
      wl_keyboard_add_listener( ctx->wlkeyboard, &essKeyboardListener, ctx );
   }
   if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
   {
      DEBUG("essSeatCapabilities:  seat has pointer\n");
      ctx->wlpointer= wl_seat_get_pointer( ctx->wlseat );
      DEBUG("essSeatCapabilities:  pointer %p\n", ctx->wlpointer );
      wl_pointer_add_listener( ctx->wlpointer, &essPointerListener, ctx );
   }
   if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
   {
      DEBUG("essSeatCapabilities:  seat has touch\n");
      ctx->wltouch= wl_seat_get_touch( ctx->wlseat );
      DEBUG("essSeatCapabilities:  touch %p\n", ctx->wltouch );
   }   
}

static void essSeatName( void *data, struct wl_seat *seat, const char *name )
{
   ESS_UNUSED(data);
   ESS_UNUSED(seat);
   ESS_UNUSED(name);
}

static const struct wl_seat_listener essSeatListener = {
   essSeatCapabilities,
   essSeatName 
};

static void essOutputGeometry( void *data, struct wl_output *output, int32_t x, int32_t y,
                               int32_t physical_width, int32_t physical_height, int32_t subpixel,
                               const char *make, const char *model, int32_t transform )
{
   ESS_UNUSED(data);
   ESS_UNUSED(output);
   ESS_UNUSED(x);
   ESS_UNUSED(y);
   ESS_UNUSED(physical_width);
   ESS_UNUSED(physical_height);
   ESS_UNUSED(subpixel);
   ESS_UNUSED(make);
   ESS_UNUSED(model);
   ESS_UNUSED(transform);
}

static void essOutputMode( void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh )
{
	EssCtx *ctx= (EssCtx*)data;

   DEBUG("essOutputMode: outputMode: mode %d(%d) (%dx%d)\n", flags, WL_OUTPUT_MODE_CURRENT, width, height);
   if ( flags & WL_OUTPUT_MODE_CURRENT )
   {
      if ( (width != ctx->planeWidth) || (height != ctx->planeHeight) )
      {
         ctx->planeWidth= width;
         ctx->planeHeight= height;
         
         wl_egl_window_resize( ctx->wleglwindow, width, height, 0, 0 );
      }
   }
}

static void essOutputDone( void *data, struct wl_output *output )
{
   ESS_UNUSED(data);
   ESS_UNUSED(output);
}

static void essOutputScale( void *data, struct wl_output *output, int32_t factor )
{
   ESS_UNUSED(data);
   ESS_UNUSED(output);
   ESS_UNUSED(factor);
}

static const struct wl_output_listener essOutputListener = {
   essOutputGeometry,
   essOutputMode,
   essOutputDone,
   essOutputScale
};

static void essRegistryHandleGlobal(void *data, 
                                    struct wl_registry *registry, uint32_t id,
		                              const char *interface, uint32_t version)
{
	EssCtx *ctx= (EssCtx*)data;
	int len;

   DEBUG("essRegistryHandleGlobal: id %d interface (%s) version %d\n", id, interface, version );

   len= strlen(interface);
   if ( (len==13) && !strncmp(interface, "wl_compositor", len) ) {
      ctx->wlcompositor= (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
      DEBUG("essRegistryHandleGlobal: wlcompositor %p\n", ctx->wlcompositor);
   } 
   else if ( (len==7) && !strncmp(interface, "wl_seat", len) ) {
      ctx->wlseat= (struct wl_seat*)wl_registry_bind(registry, id, &wl_seat_interface, 4);
      DEBUG("essRegistryHandleGlobal: wlseat %p\n", ctx->wlseat);
		wl_seat_add_listener(ctx->wlseat, &essSeatListener, ctx);
   }
   else if ( (len==9) && !strncmp(interface, "wl_output", len) ) {
      ctx->wloutput= (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
      DEBUG("essRegistryHandleGlobal: wloutput %p\n", ctx->wloutput);
      wl_output_add_listener(ctx->wloutput, &essOutputListener, ctx);
   }
}
		                              
static void essRegistryHandleGlobalRemove(void *data, 
                                          struct wl_registry *registry,
			                                 uint32_t name)
{
   ESS_UNUSED(data);
   ESS_UNUSED(registry);
   ESS_UNUSED(name);
}

static const struct wl_registry_listener essRegistryListener = 
{
	essRegistryHandleGlobal,
	essRegistryHandleGlobalRemove
};

static bool essPlatformInitWayland( EssCtx *ctx )
{
   bool result= false;

   if ( ctx )
   {
      ctx->wldisplay= wl_display_connect( NULL );
      if ( !ctx->wldisplay )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Failed to connect to wayland display" );
         goto exit;
      }

      DEBUG("essPlatformInitWayland: wldisplay %p\n", ctx->wldisplay);

      ctx->wlregistry= wl_display_get_registry( ctx->wldisplay );
      if ( !ctx->wlregistry )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Failed to get wayland display registry" );
         goto exit;
      }

      wl_registry_add_listener( ctx->wlregistry, &essRegistryListener, ctx );
      wl_display_roundtrip( ctx->wldisplay );

      ctx->waylandFd= wl_display_get_fd( ctx->wldisplay );
      if ( ctx->waylandFd < 0 )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Failed to get wayland display fd" );
         goto exit;
      }
      ctx->wlPollFd.fd= ctx->waylandFd;
      ctx->wlPollFd.events= POLLIN | POLLERR | POLLHUP;
      ctx->wlPollFd.revents= 0;

      ctx->displayType= (NativeDisplayType)ctx->wldisplay;

      result= true;
   }

exit:
   return result;
}

static void essPlatformTermWayland( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->wldisplay )
      {
         if ( ctx->wleglwindow )
         {
            wl_egl_window_destroy( ctx->wleglwindow );
            ctx->wleglwindow= 0;
         }

         if ( ctx->wlsurface )
         {
            wl_surface_destroy( ctx->wlsurface );
            ctx->wlsurface= 0;
         }

         if ( ctx->wlcompositor )
         {
            wl_compositor_destroy( ctx->wlcompositor );
            ctx->wlcompositor= 0;
         }

         if ( ctx->wlseat )
         {
            wl_seat_destroy(ctx->wlseat);
            ctx->wlseat= 0;
         }

         if ( ctx->wloutput )
         {
            wl_output_destroy(ctx->wloutput);
            ctx->wloutput= 0;
         }

         if ( ctx->wlregistry )
         {
            wl_registry_destroy( ctx->wlregistry );
            ctx->wlregistry= 0;
         }

         wl_display_disconnect( ctx->wldisplay );
         ctx->wldisplay= 0;
      }
   }
}

static void essInitInputWayland( EssCtx *ctx )
{
   if ( ctx )
   {
      #ifdef HAVE_WAYLAND
      if ( ctx->wlseat )
      {
   		wl_seat_add_listener(ctx->wlseat, &essSeatListener, ctx);
         wl_display_roundtrip(ctx->wldisplay);
      }
      #endif
   }
}

static void essProcessRunWaylandEventLoopOnce( EssCtx *ctx )
{
   int n;
   bool error= false;

   wl_display_flush( ctx->wldisplay );
   wl_display_dispatch_pending( ctx->wldisplay );

   n= poll(&ctx->wlPollFd, 1, 0);
   if ( n >= 0 )
   {
      if ( ctx->wlPollFd.revents & POLLIN )
      {
         if ( wl_display_dispatch( ctx->wldisplay ) == -1 )
         {
            error= true;
         }
      }
      if ( ctx->wlPollFd.revents & (POLLERR|POLLHUP) )
      {
         error= true;
      }
      if ( error )
      {
         if ( ctx->terminateListener && ctx->terminateListener->terminated )
         {
            ctx->terminateListener->terminated( ctx->terminateListenerUserData );
         }
      }
   }
}
#endif

#ifdef HAVE_WESTEROS
static bool essPlatformInitDirect( EssCtx *ctx )
{
   bool result= false;

   if ( ctx )
   {
      ctx->glCtx= WstGLInit();
      if ( !ctx->glCtx )
      {
         sprintf( ctx->lastErrorDetail,
                  "Error.  Failed to create a platform context" );
         goto exit;
      }
      DEBUG("essPlatformInitDirect: glCtx %p\n", ctx->glCtx);

      ctx->displayType= EGL_DEFAULT_DISPLAY;

      result= true;
   }

exit:
   return result;
}

static void essPlatformTermDirect( EssCtx *ctx )
{
   if ( ctx )
   {
      if ( ctx->glCtx )
      {
         WstGLTerm( ctx->glCtx );
         ctx->glCtx= 0;
      }
   }
}

static const char *inputPath= "/dev/input/";

static int essOpenInputDevice( EssCtx *ctx, const char *devPathName )
{
   int fd= -1;   
   struct stat buf;
   
   if ( stat( devPathName, &buf ) == 0 )
   {
      if ( S_ISCHR(buf.st_mode) )
      {
         fd= open( devPathName, O_RDONLY | O_CLOEXEC );
         if ( fd < 0 )
         {
            sprintf( ctx->lastErrorDetail,
            "Error: error opening device: %s\n", devPathName );
         }
         else
         {
            pollfd pfd;
            DEBUG( "essOpenInputDevice: opened device %s : fd %d\n", devPathName, fd );
            pfd.fd= fd;
            ctx->inputDeviceFds.push_back( pfd );
         }
      }
      else
      {
         DEBUG("essOpenInputDevice: ignoring non character device %s\n", devPathName );
      }
   }
   else
   {
      DEBUG( "essOpenInputDevice: error performing stat on device: %s\n", devPathName );
   }
   
   return fd;
}

static char *essGetInputDevice( EssCtx *ctx, const char *path, char *devName )
{
   int len;
   char *devicePathName= 0;
   struct stat buffer;
   
   if ( !devName )
      return devicePathName; 
      
   len= strlen( devName );
   
   devicePathName= (char *)malloc( strlen(path)+len+1);
   if ( devicePathName )
   {
      strcpy( devicePathName, path );
      strcat( devicePathName, devName );     
   }
   
   if ( !stat(devicePathName, &buffer) )
   {
      DEBUG( "essGetInputDevice: found %s\n", devicePathName );
   }
   else
   {
      free( devicePathName );
      devicePathName= 0;
   }
   
   return devicePathName;
}

static void essGetInputDevices( EssCtx *ctx )
{
   DIR *dir;
   struct dirent *result;
   char *devPathName;
   if ( NULL != (dir = opendir( inputPath )) )
   {
      while( NULL != (result = readdir( dir )) )
      {
         if ( (result->d_type != DT_DIR) &&
             !strncmp(result->d_name, "event", 5) )
         {
            devPathName= essGetInputDevice( ctx, inputPath, result->d_name );
            if ( devPathName )
            {
               if (essOpenInputDevice( ctx, devPathName ) >= 0 )
                  free( devPathName );
               else
                  ERROR("essos: could not open device %s\n", devPathName);
            }
         }
      }

      closedir( dir );
   }
}

static void essMonitorInputDevicesLifecycleBegin( EssCtx *ctx )
{
   pollfd pfd;

   ctx->notifyFd= inotify_init();
   if ( ctx->notifyFd >= 0 )
   {
      pfd.fd= ctx->notifyFd;
      ctx->watchFd= inotify_add_watch( ctx->notifyFd, inputPath, IN_CREATE | IN_DELETE );
      ctx->inputDeviceFds.push_back( pfd );
   }
}

static void essMonitorInputDevicesLifecycleEnd( EssCtx *ctx )
{
   if ( ctx->notifyFd >= 0 )
   {
      if ( ctx->watchFd >= 0 )
      {
         inotify_rm_watch( ctx->notifyFd, ctx->watchFd );
         ctx->watchFd= 0;
      }
      ctx->inputDeviceFds.pop_back();
      close( ctx->notifyFd );
      ctx->notifyFd= -1;
   }
}

static void essReleaseInputDevices( EssCtx *ctx )
{
   while( ctx->inputDeviceFds.size() > 0 )
   {
      pollfd pfd= ctx->inputDeviceFds[0];
      DEBUG( "essos: closing device fd: %d\n", pfd.fd );
      close( pfd.fd );
      ctx->inputDeviceFds.erase( ctx->inputDeviceFds.begin() );
   }
}

static void essProcessInputDevices( EssCtx *ctx )
{
   int deviceCount;
   int i, n;
   input_event e;
   char intfyEvent[512];
   static bool mouseMoved= false;
   static int mouseAccel= 1;
   static int mouseX= 0;
   static int mouseY= 0;

   deviceCount= ctx->inputDeviceFds.size();

   for( i= 0; i < deviceCount; ++i )
   {
      ctx->inputDeviceFds[i].events= POLLIN | POLLERR;
      ctx->inputDeviceFds[i].revents= 0;
   }

   n= poll(&ctx->inputDeviceFds[0], deviceCount, 0);
   if ( n >= 0 )
   {
      for( i= 0; i < deviceCount; ++i )
      {
         if ( ctx->inputDeviceFds[i].revents & POLLIN )
         {
            if ( ctx->inputDeviceFds[i].fd == ctx->notifyFd )
            {
               // A hotplug event has occurred
               n= read( ctx->notifyFd, &intfyEvent, sizeof(intfyEvent) );
               if ( n >= sizeof(struct inotify_event) )
               {
                  struct inotify_event *iev= (struct inotify_event*)intfyEvent;
                  if ( (iev->len >= 5) && !strncmp( iev->name, "event", 5 ) )
                  {
                     // Re-discover devices
                     DEBUG("essProcessInputDevices: inotify: mask %x (%s) wd %d (%d)\n", iev->mask, iev->name, iev->wd, ctx->watchFd );
                     pollfd pfd= ctx->inputDeviceFds.back();
                     ctx->inputDeviceFds.pop_back();
                     essReleaseInputDevices( ctx );
                     usleep( 100000 );
                     essGetInputDevices( ctx );
                     ctx->inputDeviceFds.push_back( pfd );
                     deviceCount= ctx->inputDeviceFds.size();
                  }
               }
            }
            else
            {
               n= read( ctx->inputDeviceFds[i].fd, &e, sizeof(input_event) );
               if ( n > 0 )
               {
                  switch( e.type )
                  {
                     case EV_KEY:
                        switch( e.code )
                        {
                           case BTN_LEFT:
                           case BTN_RIGHT:
                           case BTN_MIDDLE:
                           case BTN_SIDE:
                           case BTN_EXTRA:
                              {
                                 unsigned int keyCode= e.code;
                                 
                                 switch ( e.value )
                                 {
                                    case 0:
                                       essProcessPointerButtonReleased( ctx, keyCode );
                                       break;
                                    case 1:
                                       essProcessPointerButtonPressed( ctx, keyCode );
                                       break;
                                    default:
                                       break;
                                 }
                              }
                              break;
                           default:
                              {
                                 int keyCode= e.code;

                                 switch ( e.value )
                                 {
                                    case 0:
                                       essProcessKeyReleased( ctx, keyCode );
                                       break;
                                    case 1:
                                       essProcessKeyPressed( ctx, keyCode );
                                       break;
                                    default:
                                       break;
                                 }
                              }
                              break;
                        }
                        break;
                     case EV_REL:
                        switch( e.code )
                        {
                           case REL_X:
                              mouseX= mouseX + e.value * mouseAccel;
                              if ( mouseX < 0 ) mouseX= 0;
                              if ( mouseX > ctx->planeWidth ) mouseX= ctx->planeWidth;
                              mouseMoved= true;
                              break;
                           case REL_Y:
                              mouseY= mouseY + e.value * mouseAccel;
                              if ( mouseY < 0 ) mouseY= 0;
                              if ( mouseY > ctx->planeHeight ) mouseY= ctx->planeHeight;
                              mouseMoved= true;
                              break;
                           default:
                              break;
                        }
                        break;
                     case EV_SYN:
                        {
                           if ( mouseMoved )
                           {
                              essProcessPointerMotion( ctx, mouseX, mouseY );
                              
                              mouseMoved= false;
                           }
                        }
                        break;
                     default:
                        break;
                  }
               }
            }
         }
      }
   }
}
#endif

