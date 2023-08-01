#pragma once

#ifndef _PNG_IMAGE_H
#define _PNG_IMAGE_H

#include "png/base.h"
#include "png/color.h"
#include "png/compression.h"
#include "png/interlace.h"
#include "png/stream.h"

namespace PNG
{
    enum class ScalingMethod
    {
        Nearest, Bilinear,
    };

    class Image
    {
    public:
        Image(size_t width, size_t height) { SetSize(width, height); }

        Image()
            : Image(0, 0) { }

        ~Image() { delete[] m_Pixels; }

        Image(const Image& other) { *this = other; }
        Image(Image&& other) { *this = std::move(other); }

        Image& operator=(const Image& other);
        Image& operator=(Image&& other);


        void Resize(size_t width, size_t height, ScalingMethod scalingMethod = ScalingMethod::Nearest);
        void SetSize(size_t width, size_t height);
        inline void Clear() { SetSize(0, 0); }

        inline size_t GetWidth() const { return m_Width; }
        inline size_t GetHeight() const { return m_Height; }

        inline const Color* GetPixels() const { return m_Pixels; }
        inline Color* GetPixels() { return m_Pixels; }

        inline const Color* operator[](size_t y) const { return &m_Pixels[y * m_Width]; }
        inline Color* operator[](size_t y) { return &m_Pixels[y * m_Width]; }

        inline const Color& At(size_t x, size_t y, int dx = 0, int dy = 0) const
        {
            // Check if (x+dx, y+dy) is out of bounds, if true get the closest edge, otherwise get the color at that pos
            return (*this)
                [(dy > 0 || y > (size_t)-dy) * (y + dy >= m_Height ? m_Height - 1 : y + dy)]
                [(dx > 0 || x > (size_t)-dx) * (x + dx >= m_Width  ? m_Width  - 1 : x + dx)];
        }

        inline Color& At(size_t x, size_t y, int dx = 0, int dy = 0)
        {
            return (*this)
                [(dy > 0 || y > (size_t)-dy) * (y + dy >= m_Height ? m_Height - 1 : y + dy)]
                [(dx > 0 || x > (size_t)-dx) * (x + dx >= m_Width  ? m_Width  - 1 : x + dx)];
        }
        
        Result WriteRawPixels(uint8_t colorType, size_t bitDepth, OStream& out) const;
        Result WriteDitheredRawPixels(const std::vector<Color>& palette, size_t bitDepth, OStream& out) const;
        Result LoadRawPixels(uint8_t colorType, size_t bitDepth, const std::vector<Color>* palette, const std::vector<uint8_t>& in);

        Result Write(OStream& out, uint8_t colorType = ColorType::RGBA, size_t bitDepth = 8, const std::vector<Color>* palette = nullptr,
            CompressionLevel clevel = CompressionLevel::Default, uint8_t interlaceMethod = InterlaceMethod::NONE) const;
        Result WriteMT(OStream& out, uint8_t colorType = ColorType::RGBA, size_t bitDepth = 8, const std::vector<Color>* palette = nullptr,
            CompressionLevel clevel = CompressionLevel::Default, uint8_t interlaceMethod = InterlaceMethod::NONE) const;
        
        static Result Read(IStream& in, Image& out);
        static Result ReadMT(IStream& in, Image& out);

    private:
        size_t m_Width = 0;
        size_t m_Height = 0;
        Color* m_Pixels = nullptr;
    };
}

#endif // _PNG_IMAGE_H
