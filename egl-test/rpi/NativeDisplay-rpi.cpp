#include "NativeDisplay-rpi.h"

#include <cassert>
#include <iostream>
#include "bcm_host.h"

NativeDisplayRPI::NativeDisplayRPI(NativeParameters parameters)
    : _context(parameters)
{
    bcm_host_init();
}

NativeDisplayRPI::~NativeDisplayRPI()
{
    Close();
    bcm_host_deinit();
}

Size NativeDisplayRPI::GetSize() const
{
    Size result {};
    int32_t success = graphics_get_display_size(_context.deviceID, &result.width, &result.height);
    assert(success >= 0);
    std::cout << "Screen size: " << result.width << "x" << result.height << std::endl;
    return result;
}

bool NativeDisplayRPI::Open()
{
    _context.nativeDisplay = vc_dispmanx_display_open(_context.deviceID);
    if (_context.nativeDisplay == DISPMANX_NO_HANDLE)
    {
        std::cout << "Failed to open display" << std::endl;
        return false;
    }
    std::cout << "Opened Display " << _context.nativeDisplay << std::endl;
    _context.display = (EGLNativeDisplayType)(uint32_t)_context.deviceID;
    return true;
}

void NativeDisplayRPI::Close()
{
    if (_context.nativeDisplay != DISPMANX_NO_HANDLE)
    {
        std::cout << "Close display " << _context.nativeDisplay << std::endl;
        int result = vc_dispmanx_display_close(_context.nativeDisplay);
        assert (result == 0);
        _context.nativeDisplay = DISPMANX_NO_HANDLE;
        _context.display = 0;
    }
}

NativeDisplay * CreateDisplay(NativeParameters parameters)
{
    auto display = new NativeDisplayRPI(parameters);
    return display;
}
