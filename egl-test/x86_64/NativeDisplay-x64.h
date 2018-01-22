#pragma once

#include "NativeDisplay.h"
#include "drm-info/DrmInfo.h"
#include "gbm-info/GbmInfo.h"

struct NativeContextImpl
{
    size_t deviceID;
    Drm::CardInfo cardInfo;
    Gbm::Device gbmDevice;
    const Drm::ConnectorInfo * connector;
    EGLNativeDisplayType display;

    NativeContextImpl(size_t aDeviceID)
        : deviceID(aDeviceID)
        , cardInfo()
        , gbmDevice()
        , connector()
        , display()
    {}
};

class NativeDisplayX64 : public NativeDisplay
{
public:
    explicit NativeDisplayX64(NativeParameters parameters);
    ~NativeDisplayX64() override;
    Size GetSize() const override;

    bool Open() override;
    void Close() override;
    EGLNativeDisplayType GetHandle() const override { return _context.display; }
    const NativeContext * GetContext() const override { return reinterpret_cast<const NativeContext *>(&_context); }

protected:
    NativeContextImpl _context;
};

