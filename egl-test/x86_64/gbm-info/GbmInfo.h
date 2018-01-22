#pragma once

#include <string>
#include <map>
#include <vector>
#include <gbm.h>
#include <DrmInfo.h>

namespace Gbm
{

using Handle = union gbm_bo_handle;
enum class SampleFormat
{
    // Color index
        C8 = GBM_FORMAT_C8, // [7:0] C

    // 8 bpp Red
        R8 = GBM_FORMAT_R8, // [7:0] R

    // 16 bpp RG
        GR88 = GBM_FORMAT_GR88, // [15:0] G:R 8:8 little endian

    // 8 bpp RGB
        RGB332 = GBM_FORMAT_RGB332, // [7:0] R:G:B 3:3:2
    BGR233 = GBM_FORMAT_BGR233, // [7:0] B:G:R 2:3:3

    // 16 bpp RGB
        XRGB4444 = GBM_FORMAT_XRGB4444, // [15:0] x:R:G:B 4:4:4:4 little endian
    XBGR4444 = GBM_FORMAT_XBGR4444, // [15:0] x:B:G:R 4:4:4:4 little endian
    RGBX4444 = GBM_FORMAT_RGBX4444, // [15:0] R:G:B:x 4:4:4:4 little endian
    BGRX4444 = GBM_FORMAT_BGRX4444, // [15:0] B:G:R:x 4:4:4:4 little endian

    ARGB4444 = GBM_FORMAT_ARGB4444, // [15:0] A:R:G:B 4:4:4:4 little endian
    ABGR4444 = GBM_FORMAT_ABGR4444, // [15:0] A:B:G:R 4:4:4:4 little endian
    RGBA4444 = GBM_FORMAT_RGBA4444, // [15:0] R:G:B:A 4:4:4:4 little endian
    BGRA4444 = GBM_FORMAT_BGRA4444, // [15:0] B:G:R:A 4:4:4:4 little endian

    XRGB1555 = GBM_FORMAT_XRGB1555, // [15:0] x:R:G:B 1:5:5:5 little endian
    XBGR1555 = GBM_FORMAT_XBGR1555, // [15:0] x:B:G:R 1:5:5:5 little endian
    RGBX5551 = GBM_FORMAT_RGBX5551, // [15:0] R:G:B:x 5:5:5:1 little endian
    BGRX5551 = GBM_FORMAT_BGRX5551, // [15:0] B:G:R:x 5:5:5:1 little endian

    ARGB1555 = GBM_FORMAT_ARGB1555, // [15:0] A:R:G:B 1:5:5:5 little endian
    ABGR1555 = GBM_FORMAT_ABGR1555, // [15:0] A:B:G:R 1:5:5:5 little endian
    RGBA5551 = GBM_FORMAT_RGBA5551, // [15:0] R:G:B:A 5:5:5:1 little endian
    BGRA5551 = GBM_FORMAT_BGRA5551, // [15:0] B:G:R:A 5:5:5:1 little endian

    RGB565 = GBM_FORMAT_RGB565, // [15:0] R:G:B 5:6:5 little endian
    BGR565 = GBM_FORMAT_BGR565, // [15:0] B:G:R 5:6:5 little endian

    // 24 bpp RGB
        RGB888 = GBM_FORMAT_RGB888, // [23:0] R:G:B little endian
    BGR888 = GBM_FORMAT_BGR888, // [23:0] B:G:R little endian

    // 32 bpp RGB
        XRGB8888 = GBM_FORMAT_XRGB8888, // [31:0] x:R:G:B 8:8:8:8 little endian
    XBGR8888 = GBM_FORMAT_XBGR8888, // [31:0] x:B:G:R 8:8:8:8 little endian
    RGBX8888 = GBM_FORMAT_RGBX8888, // [31:0] R:G:B:x 8:8:8:8 little endian
    BGRX8888 = GBM_FORMAT_BGRX8888, // [31:0] B:G:R:x 8:8:8:8 little endian

    ARGB8888 = GBM_FORMAT_ARGB8888, // [31:0] A:R:G:B 8:8:8:8 little endian
    ABGR8888 = GBM_FORMAT_ABGR8888, // [31:0] A:B:G:R 8:8:8:8 little endian
    RGBA8888 = GBM_FORMAT_RGBA8888, // [31:0] R:G:B:A 8:8:8:8 little endian
    BGRA8888 = GBM_FORMAT_BGRA8888, // [31:0] B:G:R:A 8:8:8:8 little endian

    XRGB2101010 = GBM_FORMAT_XRGB2101010, // [31:0] x:R:G:B 2:10:10:10 little endian
    XBGR2101010 = GBM_FORMAT_XBGR2101010, // [31:0] x:B:G:R 2:10:10:10 little endian
    RGBX1010102 = GBM_FORMAT_RGBX1010102, // [31:0] R:G:B:x 10:10:10:2 little endian
    BGRX1010102 = GBM_FORMAT_BGRX1010102, // [31:0] B:G:R:x 10:10:10:2 little endian

    ARGB2101010 = GBM_FORMAT_ARGB2101010, // [31:0] A:R:G:B 2:10:10:10 little endian
    ABGR2101010 = GBM_FORMAT_ABGR2101010, // [31:0] A:B:G:R 2:10:10:10 little endian
    RGBA1010102 = GBM_FORMAT_RGBA1010102, // [31:0] R:G:B:A 10:10:10:2 little endian
    BGRA1010102 = GBM_FORMAT_BGRA1010102, // [31:0] B:G:R:A 10:10:10:2 little endian

    // packed YCbCr
        YUYV = GBM_FORMAT_YUYV, // [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian
    YVYU = GBM_FORMAT_YVYU, // [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian
    UYVY = GBM_FORMAT_UYVY, // [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian
    VYUY = GBM_FORMAT_VYUY, // [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian

    AYUV = GBM_FORMAT_AYUV, // [31:0] A:Y:Cb:Cr 8:8:8:8 little endian

    // 2 plane YCbCr
    // index 0 = Y plane, [7:0] Y
    // index 1 = Cr:Cb plane, [15:0] Cr:Cb little endian
    // or
    // index 1 = Cb:Cr plane, [15:0] Cb:Cr little endian
        NV12 = GBM_FORMAT_NV12, // 2x2 subsampled Cr:Cb plane
    NV21 = GBM_FORMAT_NV21, // 2x2 subsampled Cb:Cr plane
    NV16 = GBM_FORMAT_NV16, // 2x1 subsampled Cr:Cb plane
    NV61 = GBM_FORMAT_NV61, // 2x1 subsampled Cb:Cr plane

    // 3 plane YCbCr
    // index 0: Y plane, [7:0] Y
    // index 1: Cb plane, [7:0] Cb
    // index 2: Cr plane, [7:0] Cr
    // or
    // index 1: Cr plane, [7:0] Cr
    // index 2: Cb plane, [7:0] Cb
        YUV410 = GBM_FORMAT_YUV410, // 4x4 subsampled Cb (1) and Cr (2) planes
    YVU410 = GBM_FORMAT_YVU410, // 4x4 subsampled Cr (1) and Cb (2) planes
    YUV411 = GBM_FORMAT_YUV411, // 4x1 subsampled Cb (1) and Cr (2) planes
    YVU411 = GBM_FORMAT_YVU411, // 4x1 subsampled Cr (1) and Cb (2) planes
    YUV420 = GBM_FORMAT_YUV420, // 2x2 subsampled Cb (1) and Cr (2) planes
    YVU420 = GBM_FORMAT_YVU420, // 2x2 subsampled Cr (1) and Cb (2) planes
    YUV422 = GBM_FORMAT_YUV422, // 2x1 subsampled Cb (1) and Cr (2) planes
    YVU422 = GBM_FORMAT_YVU422, // 2x1 subsampled Cr (1) and Cb (2) planes
    YUV444 = GBM_FORMAT_YUV444, // non-subsampled Cb (1) and Cr (2) planes
    YVU444 = GBM_FORMAT_YVU444, // non-subsampled Cr (1) and Cb (2) planes
};

class Buffer
{
public:
    explicit Buffer(gbm_bo * buffer);
    Buffer(Buffer && other);
    ~Buffer();
    Buffer & operator = (Buffer && other);

    void Lock(gbm_bo * buffer);
    void Release();
    bool IsValid() const { return _buffer != nullptr; }

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetStride() const;
    SampleFormat GetFormat() const;
    gbm_device * GetDevice() const;
    Handle GetHandle() const;
    gbm_bo * InternalBuffer() const { return _buffer; }

protected:
    gbm_bo * _buffer;
};

class Surface
{
public:
    explicit Surface(gbm_surface * surface);
    ~Surface();

    bool IsValid() const { return _surface != nullptr; }

    bool NeedsLockFrontBuffer() const;
    Buffer LockFrontBuffer();
    void ReleaseBuffer(Buffer & buffer);

protected:
    gbm_surface * _surface;
};

class Device
{
public:
    Device();
    explicit Device(int fd);
    ~Device();

    bool Create(int fd);
    void Destroy();
    bool IsValid() const { return _device != nullptr; }

    gbm_device * GetDevice() const { return _device; }

    std::string GetBackendName() const;
    bool SupportsFormat(SampleFormat format, uint32_t usage) const;

    Buffer CreateBuffer(Drm::Geometry size, SampleFormat format, uint32_t usage);
    Surface CreateSurface(Drm::Geometry size, SampleFormat format, uint32_t usage);

protected:
    gbm_device * _device;
};

} // namespace Gbm

std::ostream & operator << (std::ostream & stream, Gbm::SampleFormat value);