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
        constexpr Kernel(size_t width, size_t height, std::initializer_list<double> init)
            : Width(width), Height(height), AnchorX(width / 2), AnchorY(height / 2),
              Data((Width * Height) != 0 ? new double[Width * Height] {0.0} : nullptr)
        {
            if (!Data || init.size() == 0)
                return;

            size_t i = 0;
            const size_t size = Width * Height;
            for (double value : init) {
                if (i >= size)
                    break;
                Data[i++] = value;
            }

            const double lastValue = *(init.end() - 1);
            for (; i < size; i++)
                Data[i] = lastValue;
        }

        constexpr ~Kernel() { delete[] Data; }

        constexpr const double* operator[](size_t y) const { return &Data[Width * y]; }
        constexpr double* operator[](size_t y) { return &Data[Width * y]; }
    };
}

#endif // _PNG_KERNEL_H
