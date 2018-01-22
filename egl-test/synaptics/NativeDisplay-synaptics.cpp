#include "NativeDisplay-synaptics.h"

#include <cassert>
#include <iostream>

NativeDisplaySynaptics::NativeDisplaySynaptics()
    : _context()
{
}

NativeDisplaySynaptics::~NativeDisplaySynaptics()
{
    Close();
}

Size NativeDisplaySynaptics::GetSize() const
{
    int width, height;
    fbGetDisplayGeometry(_context.display, &width, &height);
    std::cout << "Screen size: " << width << "x" << height << std::endl;
    return Size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

bool NativeDisplaySynaptics::Open()
{
    _context.display = fbGetDisplay(&_context);
    if (_context.display == nullptr)
    {
        std::cout << "Failed to open display" << std::endl;
        return false;
    }
    std::cout << "Opened Display " << _context.display << std::endl;
    return true;
}

void NativeDisplaySynaptics::Close()
{
    if (_context.display != nullptr)
    {
        std::cout << "Close display " << _context.display << std::endl;
        fbDestroyDisplay(_context.display);
        _context.display = nullptr;
    }
}

NativeDisplay * CreateDisplay(NativeParameters parameters UNUSED)
{
    auto display = new NativeDisplaySynaptics();
    return display;
}
