#pragma once

#include "egl-test/NativeWindow.h"

class NativeWindowX64 : public NativeWindow
{
public:
    NativeWindowX64();
    virtual ~NativeWindowX64();

    bool Create(const NativeContext * context, const Rect & rect) override;
    void Destroy() override;

    EGLNativeWindowType GetHandle() const override { return (EGLNativeWindowType)&_windowHandle; }

protected:
    void * _windowHandle;
};

