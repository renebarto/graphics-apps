#pragma once

#include "NativeWindow.h"

class NativeWindowRPI : public NativeWindow
{
public:
    NativeWindowRPI();
    virtual ~NativeWindowRPI();

    bool Create(const NativeContext * context, const Rect & rect) override;
    void Destroy() override;

    EGLNativeWindowType GetHandle() const override { return (EGLNativeWindowType)&_windowHandle; }

protected:
    EGL_DISPMANX_WINDOW_T _windowHandle;

    DISPMANX_UPDATE_HANDLE_T StartUpdate();
    void Commit(DISPMANX_UPDATE_HANDLE_T handle);

    DISPMANX_ELEMENT_HANDLE_T AddElement(DISPMANX_UPDATE_HANDLE_T updateHandle,
                                         DISPMANX_DISPLAY_HANDLE_T display,
                                         uint32_t layer,
                                         const VC_RECT_T & destRectangle,
                                         DISPMANX_RESOURCE_HANDLE_T srceResourceHandle,
                                         const VC_RECT_T & srceRectangle,
                                         DISPMANX_PROTECTION_T protection,
                                         VC_DISPMANX_ALPHA_T * alpha,
                                         DISPMANX_CLAMP_T * clamp,
                                         DISPMANX_TRANSFORM_T transform);
    void RemoveElement(DISPMANX_UPDATE_HANDLE_T updateHandle,
                       DISPMANX_ELEMENT_HANDLE_T handle);
};

