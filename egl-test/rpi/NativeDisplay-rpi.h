#pragma once

#include "NativeDisplay.h"

struct NativeContextImpl
{
    uint16_t deviceID;
    DISPMANX_DISPLAY_HANDLE_T nativeDisplay;
    EGLNativeDisplayType display;

    NativeContextImpl(uint16_t aDeviceID)
        : deviceID(aDeviceID)
        , nativeDisplay()
        , display()
    {}
};

class NativeDisplayRPI : public NativeDisplay
{
public:
    explicit NativeDisplayRPI(NativeParameters parameters);
    ~NativeDisplayRPI() override;
    Size GetSize() const override;

    bool Open() override;
    void Close() override;
    EGLNativeDisplayType GetHandle() const override { return _context.display; }
    const NativeContext * GetContext() const override { return reinterpret_cast<const NativeContext *>(&_context); }

protected:
    NativeContextImpl _context;
};

