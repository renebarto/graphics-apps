#include "NativeWindow-x64.h"
#include "NativeDisplay-x64.h"

#include <cassert>
#include <iostream>

NativeWindowX64::NativeWindowX64()
    : _windowHandle()
{
}

NativeWindowX64::~NativeWindowX64()
{
    Destroy();
}

bool NativeWindowX64::Create(const NativeContext * context, const Rect & rect)
{
    if (context != nullptr)
    {
        auto display = reinterpret_cast<const NativeContextImpl *>(context)->display;
        if (display != nullptr)
        {
            _windowHandle = gbm_surface_create(reinterpret_cast<gbm_device *>(display),
                                               rect.width,
                                               rect.height,
                                               GBM_FORMAT_ARGB8888,
                                               GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
        }
    }
    return _windowHandle != nullptr;
}

void NativeWindowX64::Destroy()
{
    if (_windowHandle != nullptr)
    {
        gbm_surface *gs = reinterpret_cast<gbm_surface *>(_windowHandle);

        gbm_surface_destroy(gs);

        _windowHandle = nullptr;
    }
}

NativeWindow * CreateWindow(const NativeContext * context, const Rect & rect)
{
    auto window = new NativeWindowX64;
    if (!window->Create(context, rect))
    {
        delete window;
        return nullptr;
    }
    return window;
}
