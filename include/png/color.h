#pragma once

#ifndef _PNG_COLOR_H
#define _PNG_COLOR_H

#include "png/base.h"

namespace PNG
{
    struct Color; // Forward Declaration
    using Palette_T = std::vector<Color>;

    namespace ColorType
    {
        bool IsValidBitDepth(uint8_t colorType, size_t bitDepth);
        size_t GetSamples(uint8_t colorType);
        size_t GetBytesPerSample(size_t bitDepth);
        size_t GetBytesPerPixel(uint8_t colorType, size_t bitDepth);

        const size_t MAX_SAMPLES = 4;
        
        const uint8_t GRAYSCALE = 0;
        const uint8_t RGB = 2;
        const uint8_t PALETTE = 3;
        const uint8_t GRAYSCALE_ALPHA = 4;
        const uint8_t RGBA = 6;
    }

    struct Color
    {
        float R, G, B, A;

        Color(float r, float g, float b, float a)
            : R(r), G(g), B(b), A(a) { }

        Color(float r, float g, float b)
            : Color(r, g, b, 1.0f) { }

        Color(float gray, float alpha)
            : Color(gray, gray, gray, alpha) { }

        Color(float gray)
            : Color(gray, 1.0f) { }

        Color()
            : Color(0.0f) { }

        // Constructors that accept `double` downcast to `float`

        Color(double r, double g, double b, double a)
            : Color((float)r, (float)g, (float)b, (float)a) { }

        Color(double r, double g, double b)
            : Color((float)r, (float)g, (float)b) { }

        Color(double gray, double alpha)
            : Color((float)gray, (float)alpha) { }

        Color(double gray)
            : Color((float)gray) { }

        Color& AlphaBlend(const Color& other);
        Color& Clamp();
        
        Color& operator+=(const Color& other);
        Color operator+(const Color& other) const;

        Color& operator-=(const Color& other);
        Color operator-(const Color& other) const;

        Color& operator*=(float n);
        Color operator*(float n) const;
        friend Color operator*(float n, const Color& c) { return c * n; };

        Color& operator*=(double n) { return (*this) *= (float)n; }
        Color operator*(double n) const { return (*this) * (float)n; }
        friend Color operator*(double n, const Color& c) { return c * n; };

        Color& operator/=(float n);
        Color operator/(float n) const;

        Color& operator/=(double n) { return (*this) /= (float)n; }
        Color operator/(double n) const { return (*this) / (float)n; }
    };

    size_t FindClosestPaletteColor(const Color& color, const Palette_T& palette);
}

#endif // _PNG_COLOR_H
