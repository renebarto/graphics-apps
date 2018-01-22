#pragma once

#include <cstdint>
#include "Util.h"

typedef void * NativeContext;

class NativeWindow
{
public:
    virtual ~NativeWindow() {}

    virtual bool Create(const NativeContext * context, const Rect & rect) = 0;
    virtual void Destroy() = 0;

    virtual EGLNativeWindowType GetHandle() const = 0;
};

NativeWindow * CreateWindow(const NativeContext * context, const Rect & rect);
