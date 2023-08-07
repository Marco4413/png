#include "png/color.h"

#include <algorithm>

bool PNG::ColorType::IsValidBitDepth(uint8_t colorType, uint8_t bitDepth)
{
    switch (colorType) {
    case GRAYSCALE:
        return
            bitDepth == 1 || bitDepth == 2 ||
            bitDepth == 4 || bitDepth == 8 ||
            bitDepth == 16;
    case PALETTE:
        return
            bitDepth == 1 || bitDepth == 2 ||
            bitDepth == 4 || bitDepth == 8;
    case RGB:
    case GRAYSCALE_ALPHA:
    case RGBA:
        return bitDepth == 8 || bitDepth == 16;
    default:
        return false;
    }
}

size_t PNG::ColorType::GetSamples(uint8_t colorType)
{
    switch (colorType)
    {
    case GRAYSCALE:
    case PALETTE:
        return 1;
    case GRAYSCALE_ALPHA:
        return 2;
    case RGB:
        return 3;
    case RGBA:
        return 4;
    default:
        return 0;
    }
}

size_t PNG::ColorType::GetBytesPerSample(uint8_t bitDepth)
{
    size_t bytesPerSample = bitDepth / 8;
    if (bitDepth % 8 != 0) // Round up
        return bytesPerSample + 1;
    return bytesPerSample;
}

size_t PNG::ColorType::GetBytesPerPixel(uint8_t colorType, uint8_t bitDepth)
{
    size_t samples = GetSamples(colorType);
    PNG_ASSERT(samples > 0, "PNG::ColorType::GetBytesPerPixel Invalid sample count for color.");
    PNG_ASSERT(IsValidBitDepth(colorType, bitDepth), "PNG::ColorType::GetBytesPerPixel Bit depth for color type is invalid.");
    return samples * GetBytesPerSample(bitDepth);
}

PNG::Color& PNG::Color::Clamp()
{
    R = std::clamp<float>(R, 0.0, 1.0);
    G = std::clamp<float>(G, 0.0, 1.0);
    B = std::clamp<float>(B, 0.0, 1.0);
    A = std::clamp<float>(A, 0.0, 1.0);
    return *this;
}

PNG::Color& PNG::Color::operator+=(const Color& other)
{
    R += other.R;
    G += other.G;
    B += other.B;
    A += other.A;
    return *this;
}

PNG::Color PNG::Color::operator+(const Color& other) const
{
    return std::move(Color(
        R + other.R,
        G + other.G,
        B + other.B,
        A + other.A
    ));
}

PNG::Color& PNG::Color::operator-=(const Color& other)
{
    R -= other.R;
    G -= other.G;
    B -= other.B;
    A -= other.A;
    return *this;
}

PNG::Color PNG::Color::operator-(const Color& other) const
{
    return std::move(Color(
        R - other.R,
        G - other.G,
        B - other.B,
        A - other.A
    ));
}

PNG::Color& PNG::Color::operator*=(float n)
{
    R *= n;
    G *= n;
    B *= n;
    A *= n;
    return *this;
}

PNG::Color PNG::Color::operator*(float n) const
{
    return std::move(PNG::Color(
        R * n, G * n, B * n, A * n
    ));
}

PNG::Color& PNG::Color::operator/=(float n)
{
    R /= n;
    G /= n;
    B /= n;
    A /= n;
    return *this;
}

PNG::Color PNG::Color::operator/(float n) const
{
    return std::move(Color(
        R / n, G / n, B / n, A / n
    ));
}

size_t PNG::FindClosestPaletteColor(const Color& color, const std::vector<Color>& palette)
{
    PNG_ASSERT(palette.size() > 0, "PNG::FindClosestPaletteColor Palette size is 0.");
    float bestDSq = std::numeric_limits<float>::max();
    size_t bestPaletteI = 0;

    for (size_t i = 0; i < palette.size(); i++) {
        const Color& pColor = palette[i];
        Color dColor = color - pColor;
        float dSq = dColor.R * dColor.R + dColor.G * dColor.G + dColor.B * dColor.B;
        if (dSq < bestDSq) {
            bestDSq = dSq;
            bestPaletteI = i;
        }
    }

    return bestPaletteI;
}
