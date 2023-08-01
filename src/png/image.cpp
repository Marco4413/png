#include "png/image.h"

#include "png/chunk.h"
#include "png/filter.h"

#include <cmath>
#include <thread>

PNG::Result PNG::ExportSettings::Validate() const
{
    if (IDATSize == 0)
        return Result::IllegalIDATSize;

    if (ColorType == ColorType::PALETTE) {
        if (!Palette)
            return Result::PaletteNotFound;
        size_t maxPaletteSize = (1 << BitDepth) - 1;
        if (Palette->size() >= maxPaletteSize)
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

PNG::Image& PNG::Image::operator=(const Image& other)
{
    SetSize(other.m_Width, other.m_Height);
    if (m_Pixels)
        memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Color));
    return *this;
}

PNG::Image& PNG::Image::operator=(Image&& other)
{
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Pixels = other.m_Pixels;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Pixels = nullptr;
    return *this;
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
            size_t srcy = std::floor(y * scaleY + 0.5);
            for (size_t x = 0; x < m_Width; x++) {
                size_t srcx = std::floor(x * scaleX + 0.5);
                dst[y][x] = src[srcy][srcx];
            }
        }
        break;
    // https://en.wikipedia.org/wiki/Bilinear_interpolation
    case ScalingMethod::Bilinear:
        for (size_t y = 0; y < m_Height; y++) {
            double cy = y * scaleY;
            size_t srcy = std::floor(cy);
            for (size_t x = 0; x < m_Width; x++) {
                double cx = x * scaleX;
                size_t srcx = std::floor(cx);

                const Color& col11 = src.At(srcx, srcy, 0, 0);
                const Color& col21 = src.At(srcx, srcy, 1, 0);
                const Color& col12 = src.At(srcx, srcy, 0, 1);
                const Color& col22 = src.At(srcx, srcy, 1, 1);
                
                Color colY1 = Lerp(cx-srcx, col11, col21);
                Color colY2 = Lerp(cx-srcx, col12, col22);
                dst[y][x]   = Lerp(cy-srcy, colY1, colY2);
                dst[y][x].Clamp();
            }
        }
        break;
    default:
        PNG_UNREACHABLEF("PNG::Image::Resize case missing (%d).", (int)scalingMethod);
    }
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
    m_Pixels = new Color[width * height];
}

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
                PNG_LDEBUGF("PNG::Image::LoadRawPixels palette index %ld is out of bounds (>= %ld).", paletteIndex, palette->size());
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
                rawColor[0] = (color.R + color.G + color.B) / 3.0 * MAX_SAMPLE_VALUE;
                break;
            case ColorType::RGB:
                rawColor[0] = color.R * MAX_SAMPLE_VALUE;
                rawColor[1] = color.G * MAX_SAMPLE_VALUE;
                rawColor[2] = color.B * MAX_SAMPLE_VALUE;
                break;
            case ColorType::GRAYSCALE_ALPHA:
                rawColor[0] = (color.R + color.G + color.B) / 3.0 * MAX_SAMPLE_VALUE;
                rawColor[1] = color.A * MAX_SAMPLE_VALUE;
                break;
            case ColorType::RGBA:
                rawColor[0] = color.R * MAX_SAMPLE_VALUE;
                rawColor[1] = color.G * MAX_SAMPLE_VALUE;
                rawColor[2] = color.B * MAX_SAMPLE_VALUE;
                rawColor[3] = color.A * MAX_SAMPLE_VALUE;
                break;
            case ColorType::PALETTE:
                PNG_UNREACHABLE("PNG::Image::WriteRawPixels Early return failed for Palette color type.");
            default:
                return Result::InvalidColorType;
            }

            for (size_t j = 0; j < samples; j++) {
                size_t sample = rawColor[j];
                for (size_t k = 0; k < sampleSize; k++)
                    PNG_RETURN_IF_NOT_OK(out.WriteU8, sample >> ((sampleSize-k-1) * 8));
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
    if (palette.size() == 0 || palette.size() > (size_t)(1 << bitDepth))
        return Result::InvalidPaletteSize;
    PNG_ASSERTF(bitDepth <= 8, "PNG::Image::WriteDitheredRawPixels Illegal bit depth %ld.", bitDepth);

    // https://en.wikipedia.org/wiki/Floyd–Steinberg_dithering
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
            // Best Squared Distance
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

            line[x] = bestPaletteI;
            // No need to apply error diffusion
            if (ditheringMethod == DitheringMethod::None)
                continue;

            // Apply error diffusion
            const Color& newColor = palette[bestPaletteI];
            Color quantError = color - newColor;

            switch (ditheringMethod) {
            case DitheringMethod::Floyd:
                if (x + 1 < m_Width)
                    errImg[y][x+1] += quantError * (7.0 / 16);

                if (y + 1 < m_Height) {
                    if (x > 1)
                        errImg[y+1][x-1] += quantError * (3.0 / 16);
                    errImg[y+1][x] += quantError * (5.0 / 16);
                    if (x + 1 < m_Width)
                        errImg[y+1][x+1] += quantError * (1.0 / 16);
                }
                break;
            case DitheringMethod::Atkinson:
                if (x + 1 < m_Width) {
                    errImg[y][x+1] += quantError * (1.0 / 8);
                    if (x + 2 < m_Width)
                        errImg[y][x+2] += quantError * (1.0 / 8);
                }

                if (y + 1 < m_Height) {
                    if (x > 1)
                        errImg[y+1][x-1] += quantError * (1.0 / 8);
                    errImg[y+1][x] += quantError * (1.0 / 8);
                    if (x + 1 < m_Width)
                        errImg[y+1][x+1] += quantError * (1.0 / 8);

                    if (y + 2 < m_Height)
                        errImg[y+2][x] += quantError * (1.0 / 8);
                }
                break;
            case DitheringMethod::None:
                PNG_UNREACHABLE("PNG::Image::WriteDitheredRawPixels Early continue failed for None dithering method.");
            default:
                PNG_UNREACHABLEF("PNG::Image::WriteDitheredRawPixels case missing (%d).", (int)ditheringMethod);
            }
#if 1 // Floyd Steinberg dithering

#else // Atkinson dithering
            if (x + 1 < m_Width)
                (*img)[y][x+1] += quantError * (1.0 / 8);
            if (x + 2 < m_Width)
                (*img)[y][x+2] += quantError * (1.0 / 8);

            if (y + 1 < m_Height) {
                if (x > 1)
                    (*img)[y+1][x-1] += quantError * (1.0 / 8);
                (*img)[y+1][x] += quantError * (1.0 / 8);
                if (x + 1 < m_Width)
                    (*img)[y+1][x+1] += quantError * (1.0 / 8);
            }

            if (y + 2 < m_Height)
                (*img)[y+2][x] += quantError * (1.0 / 8);
#endif
        }

        PNG_RETURN_IF_NOT_OK(out.WriteVector, line);
        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    return Result::OK;
}

PNG::Result PNG::Image::Read(IStream& in, PNG::Image& out)
{
    uint8_t sig[PNG_SIGNATURE_LEN];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, sig, PNG_SIGNATURE_LEN);
    if (memcmp(sig, PNG_SIGNATURE, PNG_SIGNATURE_LEN) != 0)
        return Result::InvalidSignature;

    // Reading IHDR (Image Header)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
    PNG_RETURN_IF_NOT_OK(IHDRChunk::Parse, chunk, ihdr);

    PNG_LDEBUGF("PNG::Image::Read Reading image %dx%d (bd=%d,ct=%d,cm=%d,fm=%d,im=%d).",
        ihdr.Width, ihdr.Height, ihdr.BitDepth, ihdr.ColorType,
        ihdr.CompressionMethod, ihdr.FilterMethod, ihdr.InterlaceMethod);

    if (ihdr.Width == 0 || ihdr.Height == 0)
        return Result::InvalidImageSize;

    // If color has 0 samples per component then it is not valid
    if (!ColorType::GetSamples(ihdr.ColorType))
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    std::vector<Color> palette;

    // Vector holding all IDATs (Deflated Image Data)
    DynamicByteStream idat;
    size_t idats = 0;

    do {
        PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
        if (chunk.CRC != chunk.CalculateCRC())
            return Result::CorruptedChunk;

        bool isAux = ChunkType::IsAncillary(chunk.Type);

        switch (chunk.Type) {
        case ChunkType::PLTE:
            if (idats > 0 || ihdr.ColorType == ColorType::GRAYSCALE ||
                ihdr.ColorType == ColorType::GRAYSCALE_ALPHA
            ) {
                return Result::IllegalPaletteChunk;
            } else if (palette.size() > 0) {
                return Result::DuplicatePalette;
            } else if (ihdr.ColorType == ColorType::PALETTE &&
                (chunk.Length() / 3) > (size_t)(1 << ihdr.BitDepth)
            ) {
                return Result::InvalidPaletteSize;
            }

            for (size_t i = 0; i < chunk.Length(); i += 3) {
                palette.emplace_back(chunk.Data[i]/255.0,
                    chunk.Data[i+1]/255.0,
                    chunk.Data[i+2]/255.0);
            }
            break;
        case ChunkType::IDAT:
            // TODO: Check if last chunk was IDAT
            if (ihdr.ColorType == ColorType::PALETTE && palette.size() == 0)
                return Result::PaletteNotFound;

            PNG_RETURN_IF_NOT_OK(idat.WriteVector, chunk.Data);
            PNG_RETURN_IF_NOT_OK(idat.Flush);
        case ChunkType::IEND:
            break;
        default:
            PNG_LDEBUGF("PNG::Image::Read Reading unknown chunk %.*s (0x%x).", (int)sizeof(chunk.Type), (char*)&chunk.Type, chunk.Type);
            if (!isAux)
                return Result::UnknownNecessaryChunk;
        }
    } while (chunk.Type != ChunkType::IEND);
    idat.Close();

    DynamicByteStream intPixels; // Interlaced Pixels

    // Inflating IDAT
    PNG_RETURN_IF_NOT_OK(DecompressData, ihdr.CompressionMethod, idat, intPixels);
    intPixels.Close();

    std::vector<uint8_t> rawPixels;

    size_t samples = ColorType::GetSamples(ihdr.ColorType);
    PNG_ASSERT(samples > 0, "Sample count is 0.");

    PNG_RETURN_IF_NOT_OK(DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
        ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, intPixels, rawPixels);

    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, &palette, rawPixels);

    return Result::OK;
}

PNG::Result PNG::Image::ReadMT(IStream& in, PNG::Image& out)
{
    uint8_t sig[PNG_SIGNATURE_LEN];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, sig, PNG_SIGNATURE_LEN);
    if (memcmp(sig, PNG_SIGNATURE, PNG_SIGNATURE_LEN) != 0)
        return Result::InvalidSignature;

    // Reading IHDR (Image Header)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
    PNG_RETURN_IF_NOT_OK(IHDRChunk::Parse, chunk, ihdr);

    PNG_LDEBUGF("PNG::Image::Read Reading image %dx%d (bd=%d,ct=%d,cm=%d,fm=%d,im=%d).",
        ihdr.Width, ihdr.Height, ihdr.BitDepth, ihdr.ColorType,
        ihdr.CompressionMethod, ihdr.FilterMethod, ihdr.InterlaceMethod);

    if (ihdr.Width == 0 || ihdr.Height == 0)
        return Result::InvalidImageSize;

    // If color has 0 samples per component then it is not valid
    if (!ColorType::GetSamples(ihdr.ColorType))
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    std::vector<Color> palette;

    DynamicByteStream idat;
    Result readerTRes;

    std::thread readerT([&in, &ihdr, &palette, &idat, &readerTRes]() {
        size_t idats = 0;

        Chunk chunk;
        do {
            readerTRes = Chunk::Read(in, chunk);
            if (readerTRes != Result::OK)
                return;
            
            if (chunk.CRC != chunk.CalculateCRC()) {
                readerTRes = Result::CorruptedChunk;
                return;
            }
            
            bool isAux = ChunkType::IsAncillary(chunk.Type);

            switch (chunk.Type) {
            case ChunkType::PLTE:
                if (idats > 0 || ihdr.ColorType == ColorType::GRAYSCALE ||
                    ihdr.ColorType == ColorType::GRAYSCALE_ALPHA
                ) {
                    readerTRes = Result::IllegalPaletteChunk;
                    return;
                } else if (palette.size() > 0) {
                    readerTRes = Result::DuplicatePalette;
                    return;
                } else if (ihdr.ColorType == ColorType::PALETTE &&
                    (chunk.Length() / 3) > (size_t)(1 << ihdr.BitDepth)
                ) {
                    readerTRes = Result::InvalidPaletteSize;
                    return;
                }

                for (size_t i = 0; i < chunk.Length(); i += 3) {
                    palette.emplace_back(chunk.Data[i]/255.0,
                        chunk.Data[i+1]/255.0,
                        chunk.Data[i+2]/255.0);
                }
                break;
            case ChunkType::IDAT:
                // TODO: Check if last chunk was IDAT
                if (ihdr.ColorType == ColorType::PALETTE && palette.size() == 0) {
                    readerTRes = Result::PaletteNotFound;
                    return;
                }

                readerTRes = idat.WriteVector(chunk.Data);
                if (readerTRes != Result::OK)
                    return;

                readerTRes = idat.Flush();
                if (readerTRes != Result::OK)
                    return;

                idats++;
            case ChunkType::IEND:
                break;
            default:
                PNG_LDEBUGF("PNG::Image::Read Reading unknown chunk %.*s (0x%x).", (int)sizeof(chunk.Type), (char*)&chunk.Type, chunk.Type);
                if (!isAux) {
                    readerTRes = Result::UnknownNecessaryChunk;
                    return;
                }
            }
        } while (chunk.Type != ChunkType::IEND);
        // While reading IDATs, PNG::DecompressData can read the buffer in another thread
    });

    DynamicByteStream intPixels; // Interlaced Pixels
    Result inflaterTRes;
    std::thread inflaterT([&ihdr, &idat, &intPixels, &inflaterTRes]() {
        // Inflating IDAT
        inflaterTRes = DecompressData(ihdr.CompressionMethod, idat, intPixels);
        // While inflating IDATs, PNG::DeinterlacePixels can execute and
        //  PNG::Image::LoadRawPixels could read the vector as it gets filled
    });
    
    std::vector<uint8_t> rawPixels;

    Result deinterlacerTRes;
    std::thread deinterlacerT([&ihdr, &intPixels, &rawPixels, &deinterlacerTRes]() {
        size_t samples = ColorType::GetSamples(ihdr.ColorType);
        PNG_ASSERT(samples > 0, "Sample count is 0.");

        deinterlacerTRes = DeinterlacePixels(ihdr.InterlaceMethod, ihdr.FilterMethod,
            ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, intPixels, rawPixels);
    });

    PNG_LDEBUG("Joining IDAT Reader.");
    readerT.join();
    idat.Close();
    PNG_LDEBUG("Joining IDAT Inflater.");
    inflaterT.join();
    intPixels.Close();
    PNG_LDEBUG("Joining Deinterlacer.");
    deinterlacerT.join();

    if (readerTRes != Result::OK)
        return readerTRes;
    if (inflaterTRes != Result::OK)
        return inflaterTRes;
    if (deinterlacerTRes != Result::OK)
        return deinterlacerTRes;

    PNG_LDEBUG("Loading raw pixels into Image.");
    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, &palette, rawPixels);

    return Result::OK;
}

PNG::Result PNG::Image::Write(OStream& out, const ExportSettings& cfg) const
{
    PNG_RETURN_IF_NOT_OK(cfg.Validate);
    PNG_RETURN_IF_NOT_OK(out.WriteBuffer, PNG_SIGNATURE, PNG_SIGNATURE_LEN);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    IHDRChunk ihdr;
    ihdr.Width = m_Width;
    ihdr.Height = m_Height;
    ihdr.BitDepth = cfg.BitDepth;
    ihdr.ColorType = cfg.ColorType;
    ihdr.CompressionMethod = CompressionMethod::ZLIB;
    ihdr.FilterMethod = FilterMethod::ADAPTIVE_FILTERING;
    ihdr.InterlaceMethod = cfg.InterlaceMethod;

    Chunk chunk;
    PNG_RETURN_IF_NOT_OK(ihdr.Write, chunk);
    PNG_RETURN_IF_NOT_OK(chunk.Write, out);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    DynamicByteStream rawImage;
    if (ihdr.ColorType == ColorType::PALETTE) {
        PNG_ASSERT(cfg.Palette, "PNG::Image::Write Early palette check failed.");
        PNG_RETURN_IF_NOT_OK(WriteDitheredRawPixels, *cfg.Palette, ihdr.BitDepth, cfg.DitheringMethod, rawImage);

        chunk.Type = ChunkType::PLTE;
        chunk.Data.resize(cfg.Palette->size() * 3);
        for (size_t i = 0; i < cfg.Palette->size(); i++) {
            const Color& color = (*cfg.Palette)[i];
            chunk.Data[i*3  ] = color.R * 255;
            chunk.Data[i*3+1] = color.G * 255;
            chunk.Data[i*3+2] = color.B * 255;
        }
        chunk.CRC = chunk.CalculateCRC();

        PNG_RETURN_IF_NOT_OK(chunk.Write, out);
        PNG_RETURN_IF_NOT_OK(out.Flush);
    } else PNG_RETURN_IF_NOT_OK(WriteRawPixels, ihdr.ColorType, ihdr.BitDepth, rawImage);
    PNG_RETURN_IF_NOT_OK(rawImage.Close);

    size_t samples = ColorType::GetSamples(cfg.ColorType);
    PNG_ASSERT(samples, "PNG::Image::Write Early color type check failed.");

    DynamicByteStream inf;
    PNG_RETURN_IF_NOT_OK(InterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
        ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, cfg.CompressionLevel, rawImage, inf);
    PNG_RETURN_IF_NOT_OK(inf.Close);

    {
        DynamicByteStream def;
        PNG_RETURN_IF_NOT_OK(CompressData, ihdr.CompressionMethod, inf, def, cfg.CompressionLevel);
        PNG_RETURN_IF_NOT_OK(def.Close);

        // Split IDAT chunks
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
            PNG_RETURN_IF_NOT_OK(out.Flush);
        }
    }

    chunk.Type = ChunkType::IEND;
    chunk.Data.resize(0);
    chunk.CRC = chunk.CalculateCRC();

    PNG_RETURN_IF_NOT_OK(chunk.Write, out);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    return Result::OK;
}

PNG::Result PNG::Image::WriteMT(OStream& out, const ExportSettings& cfg) const
{
    PNG_RETURN_IF_NOT_OK(cfg.Validate);
    PNG_RETURN_IF_NOT_OK(out.WriteBuffer, PNG_SIGNATURE, PNG_SIGNATURE_LEN);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    IHDRChunk ihdr;
    ihdr.Width = m_Width;
    ihdr.Height = m_Height;
    ihdr.BitDepth = cfg.BitDepth;
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
            PNG_ASSERT(cfg.Palette, "PNG::Image::Write Early palette check failed.");
            chunk.Type = ChunkType::PLTE;
            chunk.Data.resize(cfg.Palette->size() * 3);
            for (size_t i = 0; i < cfg.Palette->size(); i++) {
                const Color& color = (*cfg.Palette)[i];
                chunk.Data[i*3  ] = color.R * 255;
                chunk.Data[i*3+1] = color.G * 255;
                chunk.Data[i*3+2] = color.B * 255;
            }
            chunk.CRC = chunk.CalculateCRC();

            PNG_RETURN_IF_NOT_OK(chunk.Write, out);
            PNG_RETURN_IF_NOT_OK(out.Flush);
        }
    }

    DynamicByteStream rawImage;
    Result rawReaderTRes;
    std::thread rawReaderT([this, &cfg, &ihdr, &rawImage, &rawReaderTRes]() {
        if (ihdr.ColorType == ColorType::PALETTE) {
            PNG_ASSERT(cfg.Palette, "PNG::Image::Write Early palette check failed.");
            rawReaderTRes = WriteDitheredRawPixels(*cfg.Palette, ihdr.BitDepth, cfg.DitheringMethod, rawImage);
        } else rawReaderTRes = WriteRawPixels(ihdr.ColorType, ihdr.BitDepth, rawImage);
    });

    DynamicByteStream inf;
    Result interlacerTRes;
    std::thread interlacerT([&cfg, &ihdr, &rawImage, &inf, &interlacerTRes]() {
        size_t samples = ColorType::GetSamples(cfg.ColorType);
        PNG_ASSERT(samples, "PNG::Image::Write Early color type check failed.");
        interlacerTRes = InterlacePixels(ihdr.InterlaceMethod, ihdr.FilterMethod,
            ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, cfg.CompressionLevel, rawImage, inf);
    });

    DynamicByteStream def;
    Result deflaterTRes;
    std::thread deflaterT([&cfg, &ihdr, &inf, &def, &deflaterTRes]() {
        deflaterTRes = CompressData(ihdr.CompressionMethod, inf, def, cfg.CompressionLevel);
    });

    Result idatWriterTRes = Result::OK;
    std::thread idatWriterT([&cfg, &out, &def, &idatWriterTRes]() {
        // Split IDAT chunks
        Chunk chunk;
        chunk.Type = ChunkType::IDAT;

        while (true) {
            chunk.Data.resize(cfg.IDATSize);

            size_t bRead;
            auto rres = def.ReadVector(chunk.Data, &bRead);
            if (rres == Result::EndOfFile)
                break;
            else if (rres != Result::OK) {
                idatWriterTRes = rres;
                return;
            }

            chunk.Data.resize(bRead);
            chunk.CRC = chunk.CalculateCRC();

            idatWriterTRes = chunk.Write(out);
            if (idatWriterTRes != Result::OK)
                return;

            idatWriterTRes = out.Flush();
            if (idatWriterTRes != Result::OK)
                return;
        }
    });

    rawReaderT.join();
    rawImage.Close();
    interlacerT.join();
    inf.Close();
    deflaterT.join();
    def.Close();
    idatWriterT.join();

    if (rawReaderTRes != Result::OK)
        return rawReaderTRes;
    if (interlacerTRes != Result::OK)
        return interlacerTRes;
    if (deflaterTRes != Result::OK)
        return deflaterTRes;
    if (idatWriterTRes != Result::OK)
        return idatWriterTRes;

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
