#include "NativeWindow-rpi.h"
#include "NativeDisplay-rpi.h"

#include <cassert>
#include <iostream>
#include <EGL/eglplatform.h>

using namespace std;

NativeWindowRPI::NativeWindowRPI()
    : _windowHandle()
{
}

NativeWindowRPI::~NativeWindowRPI()
{
    Destroy();
}

bool NativeWindowRPI::Create(const NativeContext * context, const Rect & rect)
{
    if (context != nullptr)
    {
        auto display = reinterpret_cast<const NativeContextImpl *>(context)->nativeDisplay;
        if (display != 0)
        {
            auto updateHandle = StartUpdate();
            VC_RECT_T dst_rect;
            dst_rect.x = rect.x;
            dst_rect.y = rect.y;
            dst_rect.width = rect.width;
            dst_rect.height = rect.height;

            VC_RECT_T src_rect;
            src_rect.x = rect.x << 16;
            src_rect.y = rect.y << 16;
            src_rect.width = rect.width << 16;
            src_rect.height = rect.height << 16;

            DISPMANX_ELEMENT_HANDLE_T element = AddElement(updateHandle, display,
                                                           0 /*layer*/, dst_rect, 0 /*src*/, src_rect,
                                                           DISPMANX_PROTECTION_NONE,
                                                           nullptr /*alpha*/, nullptr /*clamp*/,
                                                           DISPMANX_TRANSFORM_T::DISPMANX_NO_ROTATE);

            _windowHandle.element = element;
            _windowHandle.width = rect.width;
            _windowHandle.height = rect.height;
            Commit(updateHandle);
            if (_windowHandle.element == 0)
            {
                cerr << "Failed to create window" << std::endl;
            } else
            {
                cout << "Created window " << _windowHandle.element << endl;
            }
        }
    }
    return _windowHandle.element != DISPMANX_NO_HANDLE;
}

void NativeWindowRPI::Destroy()
{
    if (_windowHandle.element != DISPMANX_NO_HANDLE)
    {
        cout << "Destroy window " << _windowHandle.element << endl;
        auto updateHandle = StartUpdate();
        RemoveElement(updateHandle, _windowHandle.element);
        Commit(updateHandle);
        _windowHandle.element = DISPMANX_NO_HANDLE;
    }
}

DISPMANX_UPDATE_HANDLE_T NativeWindowRPI::StartUpdate()
{
    return vc_dispmanx_update_start(0);
}

void NativeWindowRPI::Commit(DISPMANX_UPDATE_HANDLE_T handle)
{
    vc_dispmanx_update_submit_sync(handle);
}

DISPMANX_ELEMENT_HANDLE_T NativeWindowRPI::AddElement(DISPMANX_UPDATE_HANDLE_T updateHandle,
                                                      DISPMANX_DISPLAY_HANDLE_T display,
                                                      uint32_t layer,
                                                      const VC_RECT_T & destRectangle,
                                                      DISPMANX_RESOURCE_HANDLE_T srceResourceHandle,
                                                      const VC_RECT_T & srceRectangle,
                                                      DISPMANX_PROTECTION_T protection,
                                                      VC_DISPMANX_ALPHA_T * alpha,
                                                      DISPMANX_CLAMP_T * clamp,
                                                      DISPMANX_TRANSFORM_T transform)
{
    DISPMANX_ELEMENT_HANDLE_T result = vc_dispmanx_element_add(updateHandle, display,
                                                               layer, &destRectangle,
                                                               srceResourceHandle, &srceRectangle,
                                                               protection,
                                                               alpha, clamp, transform);
    return result;
}

void NativeWindowRPI::RemoveElement(DISPMANX_UPDATE_HANDLE_T updateHandle,
                                   DISPMANX_ELEMENT_HANDLE_T handle)
{
    int result = vc_dispmanx_element_remove(updateHandle, handle);
    assert(result == 0);
}

NativeWindow * CreateWindow(const NativeContext * context, const Rect & rect)
{
    auto window = new NativeWindowRPI;
    if (!window->Create(context, rect))
    {
        delete window;
        return nullptr;
    }
    return window;
}
