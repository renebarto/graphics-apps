#include "GbmInfo.h"

#include <fcntl.h>
#include <zconf.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <DrmInfo.h>

using namespace std;
using namespace Drm;
using namespace Gbm;

Buffer::Buffer(gbm_bo * buffer)
    : _buffer()
{
    Lock(buffer);
}

Buffer::Buffer(Buffer && other)
    : _buffer(std::move(other._buffer))
{}

Buffer::~Buffer()
{
    gbm_bo_destroy(_buffer);
    Release();
}

Buffer & Buffer::operator = (Buffer && other)
{
    if (this != &other)
    {
        _buffer = std::move(other._buffer);
    }
    return *this;
}

void Buffer::Lock(gbm_bo * buffer)
{
    _buffer = buffer;
}

void Buffer::Release()
{
    _buffer = nullptr;
}

uint32_t Buffer::GetWidth() const
{
    return gbm_bo_get_width(_buffer);
}

uint32_t Buffer::GetHeight() const
{
    return gbm_bo_get_height(_buffer);
}

uint32_t Buffer::GetStride() const
{
    return gbm_bo_get_stride(_buffer);
}

SampleFormat Buffer::GetFormat() const
{
    return static_cast<SampleFormat>(gbm_bo_get_format(_buffer));
}

gbm_device * Buffer::GetDevice() const
{
    return gbm_bo_get_device(_buffer);
}

Handle Buffer::GetHandle() const
{
    return gbm_bo_get_handle(_buffer);
}

Surface::Surface(gbm_surface * surface)
    : _surface(surface)
{}

Surface::~Surface()
{
    gbm_surface_destroy(_surface);
}

bool Surface::NeedsLockFrontBuffer() const
{

}

Buffer Surface::LockFrontBuffer()
{

}

void Surface::ReleaseBuffer(Buffer & buffer)
{

}

Device::Device()
    : _device()
{}

Device::Device(int fd)
    : _device()
{
    Create(fd);
}

Device::~Device()
{
    Destroy();
}

bool Device::Create(int fd)
{
    Destroy();
    _device = gbm_create_device(fd);
}

void Device::Destroy()
{
    if (IsValid())
        gbm_device_destroy(_device);
    _device = nullptr;
}

std::string Device::GetBackendName() const
{
    return gbm_device_get_backend_name(_device);
}

bool Device::SupportsFormat(SampleFormat format, uint32_t usage) const
{
    return gbm_device_is_format_supported(_device, static_cast<uint32_t>(format), usage) != 0;
}

Buffer Device::CreateBuffer(Drm::Geometry size, SampleFormat format, uint32_t usage)
{
    return Buffer(gbm_bo_create(_device, size.width, size.height, static_cast<uint32_t>(format), usage));
}

Surface Device::CreateSurface(Drm::Geometry size, SampleFormat format, uint32_t usage)
{
    return Surface(gbm_surface_create(_device, size.width, size.height, static_cast<uint32_t>(format), usage));
}

std::ostream & operator << (std::ostream & stream, Gbm::SampleFormat value)
{
    uint32_t intValue = static_cast<uint32_t>(value);
    stream << static_cast<char>((intValue >> 0) & 0xff)
           << static_cast<char>((intValue >> 8) & 0xff)
           << static_cast<char>((intValue >> 16) & 0xff)
           << static_cast<char>((intValue >> 24) & 0xff);
}