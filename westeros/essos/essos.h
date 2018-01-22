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

#ifndef __ESSOS__
#define __ESSOS__

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef struct _EssCtx EssCtx;

typedef struct _EssKeyListener
{
   void (*keyPressed)( void *userData, unsigned int key ); 
   void (*keyReleased)( void *userData, unsigned int key ); 
} EssKeyListener;

typedef struct _ESSPointerListener
{
   void (*pointerMotion)( void *userData, int x, int y );
   void (*pointerButtonPressed)( void *userData, int button, int x, int y );
   void (*pointerButtonReleased)( void *userData, int button, int x, int y );
} EssPointerListener;

typedef struct _EssTerminateLisenter
{
   void (*terminated)( void *userData );
} EssTerminateListener;

/**
 * EssContextCreate
 *
 * Create an Essos application context.
 */
EssCtx* EssContextCreate();

/**
 * EssContextDestroy
 *
 * Destroy and application instance.  If the application is running
 * it will be stopped.  All resources will be freed.
 */
void EssContextDestroy( EssCtx *ctx );

/**
 * EssContextGetLastErrorDetail
 *
 * Returns a null terminated string giving information about the
 * last error that has occurred.
 */

const char *EssContextGetLastErrorDetail( EssCtx *ctx );

/**
 * EssContextSupportWayland
 *
 * Returns true if the context supports running as a
 * Wayland application.  To configure the application to
 * run as a Wayland application call EssContextSetUseWayland.
 */
bool EssContextSupportWayland( EssCtx *ctx );

/**
 * EssContextSupportDirect
 *
 * Returns true if the context supports running as a
 * normal native EGL applicaiton.  By default a newly
 * created context will be configurd to run as a direct
 * EGL application.
 */
bool EssContextSupportDirect( EssCtx *ctx );

/**
 * EssContextSetUseWayland
 *
 * Configure an application context to run as a Wayland
 * application.  This must be called before initializing or 
 * starting the application.
 */
bool EssContextSetUseWayland( EssCtx *ctx, bool useWayland );

/**
 * EssContextGetUseWayland
 *
 * Returns true if the application context is configured to
 * run as a Wayland application.
 */
bool EssContextGetUseWayland( EssCtx *ctx );

/**
 * EssContextSetUseDirect
 *
 * Configure an application context to run as a normal direct
 * EGL application.  This must be called before initializing or 
 * starting the application.
 */
bool EssContextSetUseDirect( EssCtx *ctx, bool useWayland );

/**
 * EssContextGetUseDirect
 *
 * Returns true if the application context is configured to
 * run as a normal direct application.
 */
bool EssContextGetUseDirect( EssCtx *ctx );

/**
 * EssContextSetEGLSurfaceAttributes
 *
 * Specifies a set of EGL surface attributes to be used when creating
 * an EGL surface.  This call can be made to replace the default
 * attributes used by Essos.
 */
bool EssContextSetEGLSurfaceAttributes( EssCtx *ctx, EGLint *attrs, EGLint size );

/**
 * EssContextGetEGLSurfaceAttributes
 *
 * Returns the current set of attributes the context is configured to use
 * when creating EGL surfaces.
 */
bool EssContextGetEGLSurfaceAttributes( EssCtx *ctx, EGLint **attrs, EGLint *size );

/**
 * EssContextSetEGLContextAttributes
 *
 * Specifies a set of EGL context attributes to be used when creating
 * an EGL context.  This call can be made to replace the default
 * attributes used by Essos.
 */
bool EssContextSetEGLContextAttributes( EssCtx *ctx, EGLint *attrs, EGLint size );

/**
 * EssContextSetEGLContectAttributes
 *
 * Returns the current set of attributes the context is configured to use
 * when creating an EGL context..
 */
bool EssContextGetEGLContextAttributes( EssCtx *ctx, EGLint **attrs, EGLint *size );

/**
 * EssContextSetInitialWindowSize
 *
 * Specifies the window size to use when the application starts
 */
bool EssContextSetInitialWindowSize( EssCtx *ctx, int width, int height );

/**
 * EssContextSetSwapInterval
 *
 * Sets the EGL swap interval used by the context.  The default interval is 1.
 */
bool EssContextSetSwapInterval( EssCtx *ctx, EGLint swapInterval );

/**
 * EssContextInit
 *
 * Initialize an application context.  Inititialization will be performed
 * by EssContextStart but for use cases where it is not desired to start
 * an application context, EssContextInit must be called before methods
 * such as EssContextGetEGLDisplayType or ESSContextCreateNativeWindow
 * can be called.
 */
bool EssContextInit( EssCtx *ctx );

/**
 * EssContextGetEGLDisplayType
 *
 * Returns a NativeDisplayType value that can be used in an eglGetDisplay call
 */
bool EssContextGetEGLDisplayType( EssCtx *ctx, NativeDisplayType *displayType );

/**
 * EssContextCreateNativeWindow
 *
 * Creates a NativeWindowType value that can be used in an eglCreateWindowSurface call.
 */
bool EssContextCreateNativeWindow( EssCtx *ctx, int width, int h, NativeWindowType *nativeWindow );

/**
 * EssContextGetWaylandDisplay
 *
 * If the context is initialized and configured to run as a Wayland app. this call
 * will return the wayland display handle.
 */
void* EssContextGetWaylandDisplay( EssCtx *ctx );

/**
 * EssContextSetKeyListener
 *
 * Set a key listener (see EssKeyListener) to receive key event callbacks. Key
 * codes are Linux codes defined by linux/input.h
 */
bool EssContextSetKeyListener( EssCtx *ctx, void *userData, EssKeyListener *listener );

/**
 * EssContextSetPointerListener
 *
 * Set a pointer listener (see EssPointerListener) to receive pointer event callbacks.
 * Button codes are Linux codes defined by linux/input.h
 */
bool EssContextSetPointerListener( EssCtx *ctx, void *userData, EssPointerListener *listener );

/**
 * EssContextSetTerminateListener
 *
 * Set a terminate listener (see EssTerminateListener) to receive a callback when the\
 * application is terminating.
 */
bool EssContextSetTerminateListener( EssCtx *ctx, void *userData, EssTerminateListener *listener );

/**
 * EssContextStart
 *
 * Start an application context running.  Context initialization will be performed by this call
 * if it has not already been done with EssContextInit.  While running the EssContextRunEventLoop
 * method must be regularly called.
 */
bool EssContextStart( EssCtx *ctx );

/**
 * EssContextStop
 *
 * Stop an application context.
 */
void EssContextStop( EssCtx *ctx );

/**
 * EssContextGetDisplaySize
 *
 * Returns the width and height of the display.  For a Wayland app this will be
 * the dimensions of the wl_output of the Wayland display the application is connected to,
 */
bool EssContextGetDisplaySize( EssCtx *ctx, int *width, int *height );

/**
 * EssContextRunEventLoopOnce
 *
 * Perform event processing.  This API will not block if no events are pending.
 * It must be called regularly while the aoplication is running.
 */
void EssContextRunEventLoopOnce( EssCtx *ctx);

/**
 * EssContextUpdateDisplay
 *
 * Perform a buffer flip operation.
 */
void EssContextUpdateDisplay( EssCtx *ctx );

#endif

