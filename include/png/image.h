#pragma once

#ifndef _PNG_IMAGE_H
#define _PNG_IMAGE_H

#include "png/base.h"
#include "png/chunk.h"
#include "png/color.h"
#include "png/compression.h"
#include "png/interlace.h"
#include "png/kernel.h"
#include "png/stream.h"

namespace PNG
{
    enum class ScalingMethod
    {
        Nearest, Bilinear,
    };

    enum class DitheringMethod
    {
        None, Floyd, Atkinson
    };

    struct ExportSettings
    {
        uint8_t ColorType = PNG::ColorType::RGBA;
        size_t BitDepth = 8;
        const std::vector<Color>* Palette = nullptr;
        bool PaletteAlpha = false;
        PNG::DitheringMethod DitheringMethod = PNG::DitheringMethod::None;
        PNG::CompressionLevel CompressionLevel = PNG::CompressionLevel::Default;
        uint8_t InterlaceMethod = PNG::InterlaceMethod::NONE;
        // Split into IDAT chunks of ~32KiB
        // This is four times the size GIMP uses (and I suppose the official libpng implementation)
        // Set to -1 (or ~0) to create the least amount of IDAT chunks
        uint32_t IDATSize = 32768;

        Result Validate() const;
    };

    enum class WrapMode
    {
        None, Clamp, Repeat,
    };

    class ConstImageRowView
    {
    public:
        const Color* const Data;
        const size_t Width;
        PNG::WrapMode WrapMode;

    public:
        ConstImageRowView(const Color* data, size_t width, PNG::WrapMode wrapMode)
            : Data(data), Width(width), WrapMode(wrapMode) { }

        ConstImageRowView(const ConstImageRowView& other) = default;
        ConstImageRowView& operator=(const ConstImageRowView& other) = default;

        const Color* At(size_t x, int64_t dx) const;

        operator bool() const { return Data; }
    };

    class ImageRowView
    {
    public:
        Color* const Data;
        const size_t Width;
        PNG::WrapMode WrapMode;

    public:
        ImageRowView(Color* data, size_t width, PNG::WrapMode wrapMode)
            : Data(data), Width(width), WrapMode(wrapMode) { }

        ImageRowView(const ImageRowView& other) = default;
        ImageRowView& operator=(const ImageRowView& other) = default;

        const Color* At(size_t x, int64_t dx) const;
        Color* At(size_t x, int64_t dx);

        operator bool() const { return Data; }
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

        void Crop(size_t left, size_t top, size_t right, size_t bottom);
        void Resize(size_t width, size_t height, ScalingMethod scalingMethod = ScalingMethod::Nearest);

        void ApplyKernel(const Kernel& kernel, WrapMode wrapMode = WrapMode::None);
        void ApplyDithering(const std::vector<Color>& palette, DitheringMethod ditheringMethod);
        void ApplyGrayscale();

        void ApplyGaussianBlur(double stDev, WrapMode wrapMode = WrapMode::None) { ApplyGaussianBlur(stDev, 3 * stDev, wrapMode); }
        void ApplyGaussianBlur(double stDev, double radius, WrapMode wrapMode = WrapMode::None);
        
        void ApplySharpening(double amount = 5.0f, WrapMode wrapMode = WrapMode::None);

        void SetSize(size_t width, size_t height);
        inline void Clear() { SetSize(0, 0); }

        inline size_t GetWidth() const { return m_Width; }
        inline size_t GetHeight() const { return m_Height; }

        inline const Color* GetPixels() const { return m_Pixels; }
        inline Color* GetPixels() { return m_Pixels; }

        ConstImageRowView GetRow(size_t y, int64_t dy = 0, WrapMode wrapMode = WrapMode::None) const;
        ImageRowView GetRow(size_t y, int64_t dy = 0, WrapMode wrapMode = WrapMode::None);

        inline const Color* operator[](size_t y) const { return &m_Pixels[y * m_Width]; }
        inline Color* operator[](size_t y) { return &m_Pixels[y * m_Width]; }

        inline const Color* At(size_t x, size_t y, int64_t dx = 0, int64_t dy = 0, const Color* fallback = nullptr) const
        {
            // Check if (x+dx, y+dy) is out of bounds, if true return fallback, otherwise get the color at that pos
            const bool isInBounds = ((dy >= 0 && y + dy < m_Height) || y >= (size_t)-dy) &&
                ((dx >= 0 && x + dx < m_Width) || x >= (size_t)-dx);
            return isInBounds ? &(*this)[y+dy][x+dx] : fallback;
        }

        inline Color* At(size_t x, size_t y, int64_t dx = 0, int64_t dy = 0, Color* fallback = nullptr)
        {
            // This doesn't feel right...
            return const_cast<Color*>(static_cast<const Image*>(this)->At(x, y, dx, dy, fallback));
        }
        
        Result WriteRawPixels(uint8_t colorType, size_t bitDepth, OStream& out) const;
        Result WriteDitheredRawPixels(const std::vector<Color>& palette, size_t bitDepth, DitheringMethod ditheringMethod, OStream& out) const;
        Result LoadRawPixels(uint8_t colorType, size_t bitDepth, const std::vector<Color>* palette, const std::vector<uint8_t>& in);

        Result Write(OStream& out, const ExportSettings& cfg = ExportSettings{}) const;
        Result WriteMT(OStream& out, const ExportSettings& cfg = ExportSettings{}) const;
        
        static Result Read(IStream& in, Image& out, IHDRChunk* ihdrOut = nullptr, std::vector<Color>* paletteOut = nullptr);
        static Result ReadMT(IStream& in, Image& out, IHDRChunk* ihdrOut = nullptr, std::vector<Color>* paletteOut = nullptr);

    private:
        size_t m_Width = 0;
        size_t m_Height = 0;
        Color* m_Pixels = nullptr;
    };
}

#endif // _PNG_IMAGE_H
