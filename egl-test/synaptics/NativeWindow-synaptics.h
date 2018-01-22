#include "NativeWindow.h"

class NativeWindowSynaptics : public NativeWindow
{
public:
    NativeWindowSynaptics();
    virtual ~NativeWindowSynaptics();

    bool Create(const NativeContext * context, const Rect & rect) override;
    void Destroy() override;

    EGLNativeWindowType GetHandle() const override { return _windowHandle; }

protected:
    EGLNativeWindowType _windowHandle;
};

