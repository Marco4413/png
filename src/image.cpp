#include "png/image.h"

#include "png/chunk.h"
#include "png/filter.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <future>
#include <ranges>
#include <unordered_set>

// This is a macro since both Image::ApplyDithering and Image::WriteDitheredRawPixels need it
// https://en.wikipedia.org/wiki/Floyd–Steinberg_dithering
// https://en.wikipedia.org/wiki/Atkinson_dithering
#define PNG_FILL_DITHERING_CASES(img, qError) \
    case DitheringMethod::Floyd: \
        if (x + 1 < (img).GetWidth()) \
            (img)[y][x+1] += (qError) * (7.0 / 16); \
        if (y + 1 < (img).GetHeight()) { \
            if (x > 1) \
                (img)[y+1][x-1] += (qError) * (3.0 / 16); \
            (img)[y+1][x] += (qError) * (5.0 / 16); \
            if (x + 1 < (img).GetWidth()) \
                (img)[y+1][x+1] += (qError) * (1.0 / 16); \
        } \
        break; \
    case DitheringMethod::Atkinson: \
        if (x + 1 < (img).GetWidth()) { \
            (img)[y][x+1] += (qError) * (1.0 / 8); \
            if (x + 2 < (img).GetWidth()) \
                (img)[y][x+2] += (qError) * (1.0 / 8); \
        } \
        if (y + 1 < (img).GetHeight()) { \
            if (x > 1) \
                (img)[y+1][x-1] += (qError) * (1.0 / 8); \
            (img)[y+1][x] += (qError) * (1.0 / 8); \
            if (x + 1 < (img).GetWidth()) \
                (img)[y+1][x+1] += (qError) * (1.0 / 8); \
            if (y + 2 < (img).GetHeight()) \
                (img)[y+2][x] += (qError) * (1.0 / 8); \
        } \
        break;

#define PNG_IMAGE_GET_ROW(ImageViewClass) \
    { \
        if (!m_Pixels) \
            return ImageViewClass(nullptr, 0, wrapMode); \
        if (dy < 0 && y < (size_t)-dy) { \
            /* Out of bounds top */ \
            switch (wrapMode) { \
            case WrapMode::None: \
                return ImageViewClass(nullptr, 0, wrapMode); \
            case WrapMode::Clamp: \
                return ImageViewClass((*this)[0], m_Width, wrapMode); \
            case WrapMode::Repeat: { \
                size_t bottomOffset = ((size_t)-dy) % m_Height; \
                return ImageViewClass((*this)[m_Height - 1 - bottomOffset], m_Width, wrapMode); \
            } \
            default: \
                PNG_UNREACHABLEF("PNG_IMAGE_GET_ROW WrapMode case missing ({}) when out of bounds top.", (int)wrapMode); \
            } \
        } else if (y + dy >= m_Height) { \
            /* Out of bounds bottom */ \
            switch (wrapMode) { \
            case WrapMode::None: \
                return ImageViewClass(nullptr, 0, wrapMode); \
            case WrapMode::Clamp: \
                return ImageViewClass((*this)[m_Height-1], m_Width, wrapMode); \
            case WrapMode::Repeat: \
                return ImageViewClass((*this)[(y+dy) % m_Height], m_Width, wrapMode); \
            default: \
                PNG_UNREACHABLEF("PNG_IMAGE_GET_ROW WrapMode case missing ({}) when out of bounds bottom.", (int)wrapMode); \
            } \
        } \
        return ImageViewClass((*this)[y+dy], m_Width, wrapMode); \
    }

#define PNG_IMAGE_VIEW_AT \
    { \
        if (!Data) \
            return nullptr; \
        if (dx < 0 && x < (size_t)-dx) { \
            /* Out of bounds left */ \
            switch (WrapMode) { \
            case PNG::WrapMode::None: \
                return nullptr; \
            case PNG::WrapMode::Clamp: \
                return &Data[0]; \
            case PNG::WrapMode::Repeat: { \
                size_t rightOffset = ((size_t)-dx) % Width; \
                return &Data[Width - 1 - rightOffset]; \
            } \
            default: \
                PNG_UNREACHABLEF("PNG_IMAGE_VIEW_AT WrapMode case missing ({}) when out of bounds left.", (int)WrapMode); \
            } \
        } else if (x + dx > Width) { \
            /* Out of bounds right */ \
            switch (WrapMode) { \
            case PNG::WrapMode::None: \
                return nullptr; \
            case PNG::WrapMode::Clamp: \
                return &Data[Width-1]; \
            case PNG::WrapMode::Repeat: \
                return &Data[(x+dx) % Width]; \
            default: \
                PNG_UNREACHABLEF("PNG_IMAGE_VIEW_AT WrapMode case missing ({}) when out of bounds right.", (int)WrapMode); \
            } \
        } \
        return &Data[x+dx]; \
    }

PNG::Result PNG::ExportSettings::Validate() const
{
    if (IDATSize == 0)
        return Result::IllegalIDATSize;

    if (ColorType == ColorType::PALETTE) {
        if (!Palette)
            return Result::PaletteNotFound;
        size_t maxPaletteSize = (size_t)1 << BitDepth;
        if (Palette->size() > maxPaletteSize)
            return Result::InvalidPaletteSize;
    }
    
    size_t samples = ColorType::GetSamples(ColorType);
    if (samples == 0)
        return Result::InvalidColorType;

    if (!ColorType::IsValidBitDepth(ColorType, BitDepth))
        return Result::InvalidBitDepth;
    
    switch (InterlaceMethod) {
    case InterlaceMethod::NONE:
    case InterlaceMethod::ADAM7:
        break;
    default:
        return Result::UnknownInterlaceMethod;
    }

    return Result::OK;
}

const PNG::Color* PNG::ConstImageRowView::At(size_t x, int64_t dx) const { PNG_IMAGE_VIEW_AT }

const PNG::Color* PNG::ImageRowView::At(size_t x, int64_t dx) const { PNG_IMAGE_VIEW_AT }
PNG::Color* PNG::ImageRowView::At(size_t x, int64_t dx) { PNG_IMAGE_VIEW_AT }

PNG::Image& PNG::Image::operator=(const Image& other)
{
    SetSize(other.m_Width, other.m_Height);
    if (m_Pixels)
        memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Color));
    return *this;
}

PNG::Image& PNG::Image::operator=(Image&& other)
{
    if (m_Pixels)
        delete[] m_Pixels;
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Pixels = other.m_Pixels;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Pixels = nullptr;
    return *this;
}

void PNG::Image::Crop(size_t left, size_t top, size_t right, size_t bottom)
{
    if (left + right >= m_Width || top + bottom >= m_Height) {
        SetSize(0, 0);
        return;
    }

    Image cropped(m_Width - (left + right), m_Height - (top + bottom));
    for (size_t y = 0; y < cropped.m_Height; y++)
        memcpy(cropped[y], &(*this)[y+top][left], cropped.m_Width * sizeof(Color));
    *this = std::move(cropped);
}

void PNG::Image::Resize(size_t newWidth, size_t newHeight, ScalingMethod scalingMethod)
{
    Image src(std::move(*this));
    SetSize(newWidth, newHeight);
    Image& dst = *this;

    if (!(dst.m_Width && dst.m_Height &&
        src.m_Width && src.m_Height))
        return;
    
    double scaleX = (src.m_Width-1.0) / (m_Width-1.0);
    double scaleY = (src.m_Height-1.0) / (m_Height-1.0);
    
    switch (scalingMethod) {
    case ScalingMethod::Nearest:
        for (size_t y = 0; y < m_Height; y++) {
            size_t srcy = (size_t)std::floor(y * scaleY + 0.5);
            for (size_t x = 0; x < m_Width; x++) {
                size_t srcx = (size_t)std::floor(x * scaleX + 0.5);
                dst[y][x] = src[srcy][srcx];
            }
        }
        break;
    // https://en.wikipedia.org/wiki/Bilinear_interpolation
    case ScalingMethod::Bilinear:
        for (size_t y = 0; y < m_Height; y++) {
            double cy = y * scaleY;
            size_t srcy = (size_t)std::floor(cy);
            for (size_t x = 0; x < m_Width; x++) {
                double cx = x * scaleX;
                size_t srcx = (size_t)std::floor(cx);

                const Color* col11 = src.At(srcx, srcy, 0, 0);
                PNG_ASSERTF(col11, "PNG::Image::Resize Color at (%ld, %ld) is null.", srcx, srcy);
                const Color* col21 = src.At(srcx, srcy, 1, 0, col11);
                const Color* col12 = src.At(srcx, srcy, 0, 1, col11);
                const Color* col22 = src.At(srcx, srcy, 1, 1, col11);
                
                Color colY1 = Lerp(cx-srcx, *col11, *col21);
                Color colY2 = Lerp(cx-srcx, *col12, *col22);
                dst[y][x]   = Lerp(cy-srcy,  colY1,  colY2);
                dst[y][x].Clamp();
            }
        }
        break;
    default:
        PNG_UNREACHABLEF("PNG::Image::Resize case missing ({}).", (int)scalingMethod);
    }
}

void PNG::Image::ApplyKernel(const Kernel& kernel, WrapMode wrapMode)
{
    if (!kernel.Data)
        return;

    const Image src(std::move(*this));
    SetSize(src.m_Width, src.m_Height);

    auto imgHeight = std::ranges::iota_view((size_t)0, m_Height);
    std::for_each(std::execution::par_unseq, imgHeight.begin(), imgHeight.end(), [this, &kernel, &src, wrapMode](size_t y) {
        for (size_t x = 0; x < m_Width; x++) {
            Color finalColor(0.0, 0.0);
            for (size_t kY = 0; kY < kernel.Height; kY++) {
                const ConstImageRowView row = src.GetRow(y, (int64_t)kY - (int64_t)kernel.AnchorY, wrapMode);
                if (!row)
                    continue;
                for (size_t kX = 0; kX < kernel.Width; kX++) {
                    double value = kernel[kY][kX];
                    if (value == 0.0)
                        continue;
                    const Color* color = row.At(x, (int64_t)kX - (int64_t)kernel.AnchorX);
                    if (!color)
                        continue;
                    finalColor += (*color) * value;
                }
            }
            (*this)[y][x] = finalColor.Clamp();
        }
    });
}

void PNG::Image::ApplyDithering(const std::vector<Color>& palette, DitheringMethod ditheringMethod)
{
    for (size_t y = 0; y < m_Height; y++) {
        for (size_t x = 0; x < m_Width; x++) {
            // Clamping Color to remove error diffusion artifacts
            // This can't be a const& because of the assignment below
            const Color color = (*this)[y][x].Clamp();

            // Find closest palette color
            const Color& newColor = palette[FindClosestPaletteColor(color, palette)];

            (*this)[y][x] = newColor; // This changes `color` if it's a const&
            // No need to apply error diffusion
            if (ditheringMethod == DitheringMethod::None)
                continue;

            // Apply error diffusion
            Color quantError = color - newColor;

            switch (ditheringMethod) {
            // We do a little bit of macro magic.
            PNG_FILL_DITHERING_CASES(*this, quantError);
            case DitheringMethod::None:
                PNG_UNREACHABLE("PNG::Image::ApplyDithering Early continue failed for None dithering method.");
            default:
                PNG_UNREACHABLEF("PNG::Image::ApplyDithering case missing ({}).", (int)ditheringMethod);
            }
        }
    }
}

void PNG::Image::ApplyGrayscale()
{
    for (size_t y = 0; y < m_Height; y++) {
        for (size_t x = 0; x < m_Width; x++) {
            Color& color = (*this)[y][x];
            float grayscale = (float)((color.R + color.G + color.B) / 3.0);
            color.R = grayscale;
            color.G = grayscale;
            color.B = grayscale;
        }
    }
}

// https://en.wikipedia.org/wiki/Gaussian_blur
void PNG::Image::ApplyGaussianBlur(double stDev, double radius, WrapMode wrapMode)
{
    size_t diameter = (size_t)(2 * radius);
    Kernel kernel(diameter, diameter, {1.0});

    // double stDev = radius; // 0.84089642
    double stDev2 = stDev*stDev;

    for (size_t kY = 0; kY < kernel.Height; kY++) {
        double y = kY - (double)kernel.AnchorY;
        for (size_t kX = 0; kX < kernel.Width; kX++) {
            double x = kX - (double)kernel.AnchorX;
            double g = 1 / (2*PNG_PI*stDev2)*std::pow(PNG_E, -(x*x+y*y)/(2*stDev2));
            kernel[kY][kX] = g;
        }
    }

    ApplyKernel(kernel, wrapMode);
}

// https://en.wikipedia.org/wiki/Unsharp_masking
void PNG::Image::ApplySharpening(double amount, double radius, double threshold, WrapMode wrapMode)
{
    Image blurred = *this;
    blurred.ApplyGaussianBlur(radius / 3.0, radius, wrapMode);

    double threshold2 = threshold * threshold;

    auto imgHeight = std::ranges::iota_view((size_t)0, m_Height);
    std::for_each(std::execution::par_unseq, imgHeight.begin(), imgHeight.end(), [this, &blurred, amount, threshold2](size_t y) {
        for (size_t x = 0; x < m_Width; x++) {
            Color colorDiff = (*this)[y][x] - blurred[y][x];
            double scalarDiff = colorDiff.R * colorDiff.R + colorDiff.G * colorDiff.G + colorDiff.B * colorDiff.B + colorDiff.A * colorDiff.A;
            if (scalarDiff < threshold2)
                continue;
            (*this)[y][x] += colorDiff * amount;
            (*this)[y][x].Clamp();
        }
    });
}

void PNG::Image::SetSize(size_t width, size_t height)
{
    delete[] m_Pixels;
    if (width == 0 || height == 0)
    {
        m_Width = 0;
        m_Height = 0;
        m_Pixels = nullptr;
        return;
    }

    m_Width = width;
    m_Height = height;
    // Using calloc to initialize memory as 0, so that Pixels have an alpha of 0
    m_Pixels = (Color*)calloc(width * height, sizeof(Color));
}

PNG::ConstImageRowView PNG::Image::GetRow(size_t y, int64_t dy, WrapMode wrapMode) const { PNG_IMAGE_GET_ROW(ConstImageRowView) }
PNG::ImageRowView PNG::Image::GetRow(size_t y, int64_t dy, WrapMode wrapMode) { PNG_IMAGE_GET_ROW(ImageRowView) }

PNG::Result PNG::Image::LoadRawPixels(uint8_t colorType, size_t bitDepth, const std::vector<Color>* palette, const std::vector<uint8_t>& in)
{
    if (colorType == ColorType::PALETTE && !palette)
        return Result::PaletteNotFound;

    size_t samples = ColorType::GetSamples(colorType);
    if (samples == 0)
        return Result::InvalidColorType;
    PNG_ASSERT(samples <= ColorType::MAX_SAMPLES, "WHY?");

    if (!ColorType::IsValidBitDepth(colorType, bitDepth))
        return Result::InvalidBitDepth;
    
    size_t sampleSize = ColorType::GetBytesPerSample(bitDepth);
    PNG_ASSERT(sampleSize <= sizeof(size_t), "HOW?");
    
    size_t pixelSize = samples * sampleSize;
    if (in.size() % pixelSize != 0)
        return Result::InvalidPixelBuffer;
    
    if (m_Width * m_Height * pixelSize != in.size())
        return Result::InvalidImageSize;
    
    const size_t MAX_SAMPLE_VALUE = (1 << bitDepth) - 1;

    size_t rawColor[ColorType::MAX_SAMPLES]{0};
    size_t pixelI = 0;
    for (size_t i = 0; i < in.size(); i += pixelSize) {
        for (size_t j = 0; j < samples; j++) {
            rawColor[j] = 0;
            for (size_t k = 0; k < sampleSize; k++) {
                rawColor[j] <<= sampleSize * 8;
                rawColor[j] |= in[i+j*sampleSize+k];
            }
        }

        Color color;
        switch (colorType)
        {
        case ColorType::GRAYSCALE: {
            float gray = (rawColor[0] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.R = gray;
            color.G = gray;
            color.B = gray;
            color.A = 1.0;
            break;
        }
        case ColorType::RGB:
            color.R = (rawColor[0] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.G = (rawColor[1] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.B = (rawColor[2] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.A = 1.0;
            break;
        case ColorType::PALETTE: {
            size_t paletteIndex = rawColor[0];
            if (paletteIndex >= palette->size()) {
                PNG_LDEBUGF("PNG::Image::LoadRawPixels palette index {} is out of bounds (>= {}).", paletteIndex, palette->size());
                return Result::InvalidPaletteIndex;
            }
            color = (*palette)[paletteIndex];
            break;
        }
        case ColorType::GRAYSCALE_ALPHA: {
            float gray = (rawColor[0] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.R = gray;
            color.G = gray;
            color.B = gray;
            color.A = (rawColor[1] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            break;
        }
        case ColorType::RGBA:
            color.R = (rawColor[0] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.G = (rawColor[1] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.B = (rawColor[2] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.A = (rawColor[3] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            break;
        default:
            return Result::InvalidColorType;
        }

        m_Pixels[pixelI++] = color;
    }

    return Result::OK;
}

PNG::Result PNG::Image::WriteRawPixels(uint8_t colorType, size_t bitDepth, OStream& out) const
{
    if (colorType == ColorType::PALETTE)
        return Result::UnsupportedColorType;

    size_t samples = ColorType::GetSamples(colorType);
    if (samples == 0)
        return Result::InvalidColorType;
    PNG_ASSERT(samples <= ColorType::MAX_SAMPLES, "WHY?");

    if (!ColorType::IsValidBitDepth(colorType, bitDepth))
        return Result::InvalidBitDepth;
    
    size_t sampleSize = ColorType::GetBytesPerSample(bitDepth);
    PNG_ASSERT(sampleSize <= sizeof(size_t), "HOW?");
    
    const size_t MAX_SAMPLE_VALUE = (1 << bitDepth) - 1;
    size_t rawColor[ColorType::MAX_SAMPLES]{0};

    for (size_t y = 0; y < m_Height; y++) {
        for (size_t x = 0; x < m_Width; x++) {
            const Color& color = (*this)[y][x];
            switch (colorType)
            {
            case ColorType::GRAYSCALE:
                rawColor[0] = (size_t)((color.R + color.G + color.B) / 3.0 * MAX_SAMPLE_VALUE);
                break;
            case ColorType::RGB:
                rawColor[0] = (size_t)(color.R * MAX_SAMPLE_VALUE);
                rawColor[1] = (size_t)(color.G * MAX_SAMPLE_VALUE);
                rawColor[2] = (size_t)(color.B * MAX_SAMPLE_VALUE);
                break;
            case ColorType::GRAYSCALE_ALPHA:
                rawColor[0] = (size_t)((color.R + color.G + color.B) / 3.0 * MAX_SAMPLE_VALUE);
                rawColor[1] = (size_t)(color.A * MAX_SAMPLE_VALUE);
                break;
            case ColorType::RGBA:
                rawColor[0] = (size_t)(color.R * MAX_SAMPLE_VALUE);
                rawColor[1] = (size_t)(color.G * MAX_SAMPLE_VALUE);
                rawColor[2] = (size_t)(color.B * MAX_SAMPLE_VALUE);
                rawColor[3] = (size_t)(color.A * MAX_SAMPLE_VALUE);
                break;
            case ColorType::PALETTE:
                PNG_UNREACHABLE("PNG::Image::WriteRawPixels Early return failed for Palette color type.");
            default:
                return Result::InvalidColorType;
            }

            for (size_t j = 0; j < samples; j++) {
                size_t sample = rawColor[j];
                for (size_t k = 0; k < sampleSize; k++)
                    PNG_RETURN_IF_NOT_OK(out.WriteU8, (uint8_t)(sample >> ((sampleSize - k - 1) * 8)));
            }
        }
        // Flush on each scanline
        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    return Result::OK;
}

PNG::Result PNG::Image::WriteDitheredRawPixels(const std::vector<Color>& palette, size_t bitDepth, DitheringMethod ditheringMethod, OStream& out) const
{
    if (!ColorType::IsValidBitDepth(ColorType::PALETTE, bitDepth))
        return Result::InvalidBitDepth;
    if (palette.size() == 0 || palette.size() > (size_t)1 << bitDepth)
        return Result::InvalidPaletteSize;
    PNG_ASSERTF(bitDepth <= 8, "PNG::Image::WriteDitheredRawPixels Illegal bit depth %ld.", bitDepth);

    // Creating a copy of the image for error diffusion
    Image errImg(0, 0);
    if (ditheringMethod != DitheringMethod::None)
        errImg = *this;

    std::vector<uint8_t> line(m_Width);
    for (size_t y = 0; y < m_Height; y++) {
        for (size_t x = 0; x < m_Width; x++) {
            // Clamping Color to remove error diffusion artifacts
            const Color& color = ditheringMethod == DitheringMethod::None ?
                (*this)[y][x] : errImg[y][x].Clamp();

            // Find closest palette color
            size_t bestPaletteI = FindClosestPaletteColor(color, palette);

            line[x] = (uint8_t)bestPaletteI;
            // No need to apply error diffusion
            if (ditheringMethod == DitheringMethod::None)
                continue;

            // Apply error diffusion
            const Color& newColor = palette[bestPaletteI];
            Color quantError = color - newColor;

            switch (ditheringMethod) {
            // We do a little more bits of macro magic.
            PNG_FILL_DITHERING_CASES(errImg, quantError);
            case DitheringMethod::None:
                PNG_UNREACHABLE("PNG::Image::WriteDitheredRawPixels Early continue failed for None dithering method.");
            default:
                PNG_UNREACHABLEF("PNG::Image::WriteDitheredRawPixels case missing ({}).", (int)ditheringMethod);
            }
        }

        PNG_RETURN_IF_NOT_OK(out.WriteVector, line);
        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    return Result::OK;
}

PNG::Result PNG::Image::Read(IStream& in, PNG::Image& out, IHDRChunk* ihdrOut, std::vector<Color>* paletteOut, bool async)
{
    auto launchPolicy = async ? std::launch::async : std::launch::deferred;

    uint8_t sig[PNG_SIGNATURE_LEN];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, sig, PNG_SIGNATURE_LEN);
    if (memcmp(sig, PNG_SIGNATURE, PNG_SIGNATURE_LEN) != 0)
        return Result::InvalidSignature;

    // Reading IHDR (Image Header)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
    PNG_RETURN_IF_NOT_OK(IHDRChunk::Parse, chunk, ihdr);

    PNG_LDEBUGF("PNG::Image::Read Reading image {}x{} (bd={},ct={},cm={},fm={},im={}).",
        ihdr.Width, ihdr.Height, ihdr.BitDepth, ihdr.ColorType,
        ihdr.CompressionMethod, ihdr.FilterMethod, ihdr.InterlaceMethod);

    if (ihdr.Width == 0 || ihdr.Height == 0)
        return Result::InvalidImageSize;

    // If color has 0 samples per component then it is not valid
    size_t samples = ColorType::GetSamples(ihdr.ColorType);
    if (samples == 0)
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    std::vector<Color> palette;

    // Deflated Image Data
    DynamicByteStream idat;
    auto reader = std::async(launchPolicy, [&in, &ihdr, &palette, &idat]() {
        std::unordered_set<uint32_t> chunkTypesRead;
        uint32_t lastChunkType = ChunkType::IHDR;

        Chunk chunk;
        do {
            PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
            if (chunk.CRC != chunk.CalculateCRC())
                return Result::CorruptedChunk;
            
            bool isAux = ChunkType::IsAncillary(chunk.Type);

            switch (chunk.Type) {
            case ChunkType::IHDR:
                return Result::IllegalIHDRChunk;
            case ChunkType::PLTE: {
                if (chunkTypesRead.contains(ChunkType::PLTE) ||
                    chunkTypesRead.contains(ChunkType::IDAT) ||
                    ihdr.ColorType == ColorType::GRAYSCALE ||
                    ihdr.ColorType == ColorType::GRAYSCALE_ALPHA
                ) {
                    return Result::IllegalPaletteChunk;
                } else if (palette.size() > 0)
                    return Result::DuplicatePalette;
                
                size_t paletteEntries = chunk.Length() / 3;
                if (ihdr.ColorType == ColorType::PALETTE &&
                    (paletteEntries == 0 ||
                    paletteEntries > (size_t)1 << ihdr.BitDepth)
                ) {
                    return Result::InvalidPaletteSize;
                }

                for (size_t i = 0; i < chunk.Length(); i += 3) {
                    palette.emplace_back(chunk.Data[i]/255.0,
                        chunk.Data[i+1]/255.0,
                        chunk.Data[i+2]/255.0);
                }
                break;
            }
            case ChunkType::IDAT:
                if (chunkTypesRead.contains(ChunkType::IDAT) &&
                    lastChunkType != ChunkType::IDAT
                ) {
                    return Result::IllegalIDATChunk;
                } else if (ihdr.ColorType == ColorType::PALETTE &&
                    !chunkTypesRead.contains(ChunkType::PLTE)
                ) {
                    return Result::PaletteNotFound;
                }

                PNG_RETURN_IF_NOT_OK(idat.WriteVector, chunk.Data);
                PNG_RETURN_IF_NOT_OK(idat.Flush);
            case ChunkType::IEND:
                break;
            case ChunkType::tRNS:
                if (chunkTypesRead.contains(ChunkType::tRNS) ||
                    chunkTypesRead.contains(ChunkType::IDAT) ||
                    ihdr.ColorType == ColorType::GRAYSCALE_ALPHA ||
                    ihdr.ColorType == ColorType::RGBA
                ) {
                    return Result::IllegaltRNSChunk;
                } else if (ihdr.ColorType != ColorType::PALETTE) {
                    PNG_LDEBUGF("PNG::Image::Read tRNS chunk is only supported for Palette color type, got {}.", (size_t)ihdr.ColorType);
                    break;
                } else if (!chunkTypesRead.contains(ChunkType::PLTE)) {
                    return Result::IllegaltRNSChunk;
                } else if (chunk.Length() > palette.size())
                    return Result::InvalidtRNSSize;

                for (size_t i = 0; i < chunk.Length(); i++)
                    palette[i].A = (float)(chunk.Data[i] / 255.0);
                break;
            default:
                PNG_LDEBUGF("PNG::Image::Read Reading unknown chunk {:.4} (0x{:x}).", (char*)&chunk.Type, chunk.Type);
                if (!isAux)
                    return Result::UnknownNecessaryChunk;
            }

            chunkTypesRead.insert(chunk.Type);
            lastChunkType = chunk.Type;
        } while (chunk.Type != ChunkType::IEND);
        // While reading IDATs, PNG::DecompressData can read the buffer in another thread
        return Result::OK;
    });

    DynamicByteStream intPixels; // Interlaced Pixels
    // Inflating IDAT
    auto inflater = std::async(launchPolicy, DecompressData, ihdr.CompressionMethod, std::ref(idat), std::ref(intPixels));
    // While inflating IDATs, PNG::DeinterlacePixels can execute and
    //  PNG::Image::LoadRawPixels could read the vector as it gets filled
    
    std::vector<uint8_t> rawPixels;
    auto deinterlacer = std::async(launchPolicy, DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
        ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, std::ref(intPixels), std::ref(rawPixels));

    PNG_LDEBUG("PNG::Image::Read Waiting for IDAT Reader.");
    reader.wait();
    idat.Close();

    PNG_LDEBUG("PNG::Image::Read Waiting for IDAT Inflater.");
    inflater.wait();
    intPixels.Close();

    PNG_LDEBUG("PNG::Image::Read Waiting for Deinterlacer.");
    deinterlacer.wait();

    PNG_RETURN_IF_NOT_OK(reader.get);
    PNG_RETURN_IF_NOT_OK(inflater.get);
    PNG_RETURN_IF_NOT_OK(deinterlacer.get);

    PNG_LDEBUG("PNG::Image::Read Loading raw pixels into Image.");
    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, &palette, rawPixels);

    if (ihdrOut)
        *ihdrOut = ihdr;
    if (paletteOut)
        *paletteOut = std::move(palette);

    return Result::OK;
}

PNG::Result PNG::Image::Write(OStream& out, const ExportSettings& cfg, bool async) const
{
    auto launchPolicy = async ? std::launch::async : std::launch::deferred;

    PNG_RETURN_IF_NOT_OK(cfg.Validate);
    PNG_RETURN_IF_NOT_OK(out.WriteBuffer, PNG_SIGNATURE, PNG_SIGNATURE_LEN);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    IHDRChunk ihdr;
    ihdr.Width = (uint32_t)m_Width;
    ihdr.Height = (uint32_t)m_Height;
    ihdr.BitDepth = (uint8_t)cfg.BitDepth;
    ihdr.ColorType = cfg.ColorType;
    ihdr.CompressionMethod = CompressionMethod::ZLIB;
    ihdr.FilterMethod = FilterMethod::ADAPTIVE_FILTERING;
    ihdr.InterlaceMethod = cfg.InterlaceMethod;

    {
        Chunk chunk;
        PNG_RETURN_IF_NOT_OK(ihdr.Write, chunk);
        PNG_RETURN_IF_NOT_OK(chunk.Write, out);
        PNG_RETURN_IF_NOT_OK(out.Flush);

        if (ihdr.ColorType == ColorType::PALETTE) {
            std::vector<uint8_t> tRNS(cfg.Palette->size(), 255);
            size_t lastAlpha = tRNS.size();

            PNG_ASSERT(cfg.Palette, "PNG::Image::Write Early palette check failed.");
            chunk.Type = ChunkType::PLTE;
            chunk.Data.resize(cfg.Palette->size() * 3);
            for (size_t i = 0; i < cfg.Palette->size(); i++) {
                const Color& color = (*cfg.Palette)[i];
                chunk.Data[i*3  ] = (uint8_t)(color.R * 255);
                chunk.Data[i*3+1] = (uint8_t)(color.G * 255);
                chunk.Data[i*3+2] = (uint8_t)(color.B * 255);
                if (cfg.PaletteAlpha && color.A < 1.0) {
                    lastAlpha = i;
                    tRNS[i] = (uint8_t)(color.A * 255);
                }
            }
            chunk.CRC = chunk.CalculateCRC();

            PNG_RETURN_IF_NOT_OK(chunk.Write, out);
            PNG_RETURN_IF_NOT_OK(out.Flush);

            // lastAlpha is changed only if cfg.PaletteAlpha is true, so we do not need to check that option
            if (lastAlpha < tRNS.size()) {
                tRNS.resize(lastAlpha+1);
                chunk.Type = ChunkType::tRNS;
                chunk.Data = std::move(tRNS);
                chunk.CRC = chunk.CalculateCRC();
                PNG_RETURN_IF_NOT_OK(chunk.Write, out);
                PNG_RETURN_IF_NOT_OK(out.Flush);
            }
        }
    }

    DynamicByteStream rawImage;
    std::future<Result> rawWriter;
    if (ihdr.ColorType == ColorType::PALETTE) {
        PNG_ASSERT(cfg.Palette, "PNG::Image::Write Early palette check failed.");
        rawWriter = std::async(launchPolicy, &Image::WriteDitheredRawPixels, this, std::ref(*cfg.Palette),
            (size_t)ihdr.BitDepth, cfg.DitheringMethod, std::ref(rawImage));
    } else
        rawWriter = std::async(launchPolicy, &Image::WriteRawPixels, this, ihdr.ColorType, ihdr.BitDepth, std::ref(rawImage));

    size_t samples = ColorType::GetSamples(cfg.ColorType);
    PNG_ASSERT(samples, "PNG::Image::Write Early color type check failed.");

    DynamicByteStream inf;
    auto interlacer = std::async(launchPolicy, InterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
            ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, cfg.CompressionLevel, std::ref(rawImage), std::ref(inf));

    DynamicByteStream def;
    auto deflater = std::async(launchPolicy, CompressData, ihdr.CompressionMethod, std::ref(inf), std::ref(def), cfg.CompressionLevel);

    auto idatWriter = std::async(launchPolicy, [&cfg, &out, &def]() {
        // Split IDAT chunks
        Chunk chunk;
        chunk.Type = ChunkType::IDAT;

        while (true) {
            chunk.Data.resize(cfg.IDATSize);

            size_t bRead;
            auto rres = def.ReadVector(chunk.Data, &bRead);
            if (rres == Result::EndOfFile)
                break;
            else if (rres != Result::OK)
                return rres;

            chunk.Data.resize(bRead);
            chunk.CRC = chunk.CalculateCRC();

            PNG_RETURN_IF_NOT_OK(chunk.Write, out);
            return out.Flush();
        }

        return Result::OK;
    });

    rawWriter.wait();
    rawImage.Close();

    interlacer.wait();
    inf.Close();

    deflater.wait();
    def.Close();

    idatWriter.wait();

    PNG_RETURN_IF_NOT_OK(rawWriter.get);
    PNG_RETURN_IF_NOT_OK(interlacer.get);
    PNG_RETURN_IF_NOT_OK(deflater.get);
    PNG_RETURN_IF_NOT_OK(idatWriter.get);

    {
        Chunk chunk;
        chunk.Type = ChunkType::IEND;
        chunk.Data.resize(0);
        chunk.CRC = chunk.CalculateCRC();
        PNG_RETURN_IF_NOT_OK(chunk.Write, out);
        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    return Result::OK;
}
