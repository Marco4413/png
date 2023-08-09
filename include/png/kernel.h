#pragma once

#ifndef _PNG_KERNEL_H
#define _PNG_KERNEL_H

#include "png/base.h"

namespace PNG
{
    class Kernel
    {
    public:
        const size_t Width;
        const size_t Height;
        const size_t AnchorX;
        const size_t AnchorY;
        double* const Data;
    public:
        Kernel(size_t width, size_t height, std::initializer_list<double> init);
        ~Kernel() { delete[] Data; }

        const double* operator[](size_t y) const { return &Data[Width * y]; }
        double* operator[](size_t y) { return &Data[Width * y]; }
    };
}

#endif // _PNG_KERNEL_H
