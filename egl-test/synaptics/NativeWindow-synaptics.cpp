#include "NativeWindow-synaptics.h"
#include "NativeDisplay-synaptics.h"

#include <cassert>
#include <iostream>

using namespace std;

NativeWindowSynaptics::NativeWindowSynaptics()
    : _windowHandle()
{
}

NativeWindowSynaptics::~NativeWindowSynaptics()
{
    Destroy();
}

bool NativeWindowSynaptics::Create(const NativeContext * context, const Rect & rect)
{
    if (context != nullptr)
    {
        auto display = reinterpret_cast<const NativeContextImpl *>(context)->display;
        if (display != nullptr)
        {
            _windowHandle = fbCreateWindow(display, rect.x, rect.y, rect.width, rect.height);
            cout << "Created window " << _windowHandle << endl;
        }
    }
    return _windowHandle != nullptr;
}

void NativeWindowSynaptics::Destroy()
{
    if (_windowHandle != nullptr)
    {
        cout << "Destroy window " << _windowHandle << endl;
        fbDestroyWindow(_windowHandle);
        _windowHandle = nullptr;
    }
}

NativeWindow * CreateWindow(const NativeContext * context, const Rect & rect)
{
    auto window = new NativeWindowSynaptics;
    if (!window->Create(context, rect))
    {
        delete window;
        return nullptr;
    }
    return window;
}
