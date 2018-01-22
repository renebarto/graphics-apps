#include "NativeDisplay-x64.h"

#include <cassert>
#include <iostream>

using namespace std;

NativeDisplayX64::NativeDisplayX64(NativeParameters parameters)
    : _context(parameters)
{
    _context.cardInfo = Drm::CardInfo::OpenCard(0);
    _context.connector = _context.cardInfo.GetPrimaryConnector();
}

NativeDisplayX64::~NativeDisplayX64()
{
    Close();
}

Size NativeDisplayX64::GetSize() const
{
    Size result {};
    if (_context.connector)
    {
        Drm::Rect rc = _context.connector->GetRect();
        cout << "  Type: " << _context.connector->GetType() << " (" << rc.x << "," << rc.y << ","
             << rc.width << "," << rc.height << ")" << endl;
        result.width = rc.width;
        result.height = rc.height;
        std::cout << "Screen size: " << result.width << "x" << result.height << std::endl;
    }
    return result;
}

bool NativeDisplayX64::Open()
{
    // We need to get a GBM buffer?
    if (_context.gbmDevice.Create(_context.cardInfo.GetFD()))
        _context.display = (EGLNativeDisplayType)_context.gbmDevice.GetDevice();
    if (_context.display == nullptr)
    {
        cout << "Failed to open display" << std::endl;
        return false;
    }
    cout << "Display opened, handle " << _context.display << std::endl;
    return true;
}

void NativeDisplayX64::Close()
{
    if (_context.display != nullptr)
    {
        _context.gbmDevice.Destroy();
        _context.display = nullptr;
    }
}

NativeDisplay * CreateDisplay(NativeParameters parameters)
{
    auto display = new NativeDisplayX64(parameters);
    return display;
}
