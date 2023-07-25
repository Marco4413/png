#pragma once

#ifndef _PNG_IMAGE_H
#define _PNG_IMAGE_H

#include "png/base.h"
#include "png/stream.h"

namespace PNG
{
    namespace ColorType
    {
        bool IsValidBitDepth(uint8_t colorType, uint8_t bitDepth);
        size_t GetSamples(uint8_t colorType);
        size_t GetBytesPerSample(uint8_t bitDepth);
        size_t GetBytesPerPixel(uint8_t colorType, uint8_t bitDepth);

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
    };

    class Image
    {
    private:
        size_t m_Width = 0;
        size_t m_Height = 0;
        Color* m_Pixels = nullptr;
    public:
        Image(size_t width, size_t height) { SetSize(width, height); }

        Image()
            : Image(0, 0) { }

        Image(const Image& other);
        Image(Image&& other);

        virtual ~Image() { delete[] m_Pixels; }

        void SetSize(size_t width, size_t height);
        inline void Clear() { SetSize(0, 0); }

        inline size_t GetWidth() const { return m_Width; }
        inline size_t GetHeight() const { return m_Height; }

        inline const Color* GetPixels() const { return m_Pixels; }
        inline Color* GetPixels() { return m_Pixels; }

        inline const Color* operator[](size_t y) const { return &m_Pixels[y * m_Width]; }
        inline Color* operator[](size_t y) { return &m_Pixels[y * m_Width]; }
        
        Result LoadRawPixels(size_t samples, size_t bitDepth, std::vector<Color>& palette, std::vector<uint8_t>& in);
        
        static Result Read(IStream& in, Image& out);
        static Result ReadMT(IStream& in, Image& out);
    };
}

#endif // _PNG_IMAGE_H
