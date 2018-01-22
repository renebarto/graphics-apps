#pragma once

#include <cstdint>
#include "Util.h"

#if defined(SYNAPTICS_PLATFORM)
typedef void * NativeParameters;
#elif defined(RPI_PLATFORM)
typedef uint16_t NativeParameters;
#elif defined(X86_64_PLATFORM)
typedef size_t NativeParameters;
#else
#error "Unsupported platform"
#endif
typedef void * NativeContext;

class NativeDisplay
{
public:
    virtual ~NativeDisplay() {}

    virtual bool Open() = 0;
    virtual void Close() = 0;

    virtual Size GetSize() const = 0;

    virtual EGLNativeDisplayType GetHandle() const = 0;
    virtual const NativeContext * GetContext() const = 0;
};

NativeDisplay * CreateDisplay(NativeParameters parameters);
