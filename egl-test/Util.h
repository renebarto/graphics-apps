#pragma once

#include "EGL/eglplatform.h"

#define UNUSED __attribute__((unused))
struct Size
{
    uint32_t width;
    uint32_t height;

    Size()
        : width()
        , height()
    {}
    Size(uint32_t aWidth, uint32_t aHeight)
        : width(aWidth)
        , height(aHeight)
    {}
};

struct Rect
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    Rect()
        : x()
        , y()
        , width()
        , height()
    {}
    Rect(uint32_t aX, uint32_t aY, uint32_t aWidth, uint32_t aHeight)
        : x(aX)
        , y(aY)
        , width(aWidth)
        , height(aHeight)
    {}
};

