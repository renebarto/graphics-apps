#pragma once

#include "NativeDisplay.h"

#include <EGL/egl.h>

struct NativeParametersImpl
{
};
struct NativeContextImpl
{
    EGLNativeDisplayType display;
};

class NativeDisplaySynaptics : public NativeDisplay
{
public:
    NativeDisplaySynaptics();
    virtual ~NativeDisplaySynaptics();

    bool Open() override;
    void Close() override;

    Size GetSize() const override;
    EGLNativeDisplayType GetHandle() const override { return _context.display; }
    const NativeContext * GetContext() const override { return reinterpret_cast<const NativeContext *>(&_context); }

protected:
    NativeContextImpl _context;
};

