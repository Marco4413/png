#include "png/image.h"

#include "png/chunk.h"
#include "png/filter.h"

#include <thread>

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

PNG::Image::Image(const PNG::Image& other)
    : PNG::Image(other.m_Width, other.m_Height)
{
    if (other.m_Pixels)
        memcpy(m_Pixels, other.m_Pixels, m_Width * m_Height * sizeof(Color));
}

PNG::Image::Image(PNG::Image&& other)
{
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Pixels = other.m_Pixels;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_Pixels = nullptr;
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

PNG::Result PNG::Image::LoadRawPixels(uint8_t colorType, size_t bitDepth, std::vector<Color>& palette, std::vector<uint8_t>& in)
{
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
            color.A = 1.0f;
            break;
        }
        case ColorType::RGB:
            color.R = (rawColor[0] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.G = (rawColor[1] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.B = (rawColor[2] & MAX_SAMPLE_VALUE) / (float)MAX_SAMPLE_VALUE;
            color.A = 1.0f;
            break;
        case ColorType::PALETTE: {
            size_t paletteIndex = rawColor[0];
            if (paletteIndex >= palette.size()) {
                PNG_LDEBUGF("PNG::Image::LoadRawPixels palette index %ld is out of bounds (>= %ld).", paletteIndex, palette.size());
                return Result::InvalidPaletteIndex;
            }
            color = palette[paletteIndex];
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
                rawColor[0] = (color.R + color.G + color.B) / 3.0f * MAX_SAMPLE_VALUE;
                break;
            case ColorType::RGB:
                rawColor[0] = color.R * MAX_SAMPLE_VALUE;
                rawColor[1] = color.G * MAX_SAMPLE_VALUE;
                rawColor[2] = color.B * MAX_SAMPLE_VALUE;
                break;
            case ColorType::GRAYSCALE_ALPHA:
                rawColor[0] = (color.R + color.G + color.B) / 3.0f * MAX_SAMPLE_VALUE;
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
                palette.emplace_back(chunk.Data[i]/255.0f,
                    chunk.Data[i+1]/255.0f,
                    chunk.Data[i+2]/255.0f);
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
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, palette, rawPixels);

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
                    palette.emplace_back(chunk.Data[i]/255.0f,
                        chunk.Data[i+1]/255.0f,
                        chunk.Data[i+2]/255.0f);
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
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, palette, rawPixels);

    return Result::OK;
}

PNG::Result PNG::Image::Write(OStream& out, uint8_t colorType, size_t bitDepth, CompressionLevel clevel, uint8_t interlaceMethod) const
{
    size_t samples = ColorType::GetSamples(colorType);
    if (samples == 0)
        return Result::InvalidColorType;

    if (!ColorType::IsValidBitDepth(colorType, bitDepth))
        return Result::InvalidBitDepth;

    PNG_RETURN_IF_NOT_OK(out.WriteBuffer, PNG_SIGNATURE, PNG_SIGNATURE_LEN);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    IHDRChunk ihdr;
    ihdr.Width = m_Width;
    ihdr.Height = m_Height;
    ihdr.BitDepth = bitDepth;
    ihdr.ColorType = colorType;
    ihdr.CompressionMethod = CompressionMethod::ZLIB;
    ihdr.FilterMethod = FilterMethod::ADAPTIVE_FILTERING;
    ihdr.InterlaceMethod = interlaceMethod;

    Chunk chunk;
    PNG_RETURN_IF_NOT_OK(ihdr.Write, chunk);
    PNG_RETURN_IF_NOT_OK(chunk.Write, out);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    DynamicByteStream rawImage;
    PNG_RETURN_IF_NOT_OK(WriteRawPixels, ihdr.ColorType, ihdr.BitDepth, rawImage);
    PNG_RETURN_IF_NOT_OK(rawImage.Close);

    DynamicByteStream inf;
    PNG_RETURN_IF_NOT_OK(InterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
        ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, clevel, rawImage, inf);
    PNG_RETURN_IF_NOT_OK(inf.Close);

    {
        DynamicByteStream def;
        PNG_RETURN_IF_NOT_OK(CompressData, ihdr.CompressionMethod, inf, def, clevel);
        
        chunk.Type = ChunkType::IDAT;
        chunk.Data = std::move(def.GetBuffer());
        chunk.CRC = chunk.CalculateCRC();
    }

    PNG_RETURN_IF_NOT_OK(chunk.Write, out);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    chunk.Type = ChunkType::IEND;
    chunk.Data.resize(0);
    chunk.CRC = chunk.CalculateCRC();

    PNG_RETURN_IF_NOT_OK(chunk.Write, out);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    return Result::OK;
}

PNG::Result PNG::Image::WriteMT(OStream& out, uint8_t colorType, size_t bitDepth, CompressionLevel clevel, uint8_t interlaceMethod) const
{
    size_t samples = ColorType::GetSamples(colorType);
    if (samples == 0)
        return Result::InvalidColorType;

    if (!ColorType::IsValidBitDepth(colorType, bitDepth))
        return Result::InvalidBitDepth;

    PNG_RETURN_IF_NOT_OK(out.WriteBuffer, PNG_SIGNATURE, PNG_SIGNATURE_LEN);
    PNG_RETURN_IF_NOT_OK(out.Flush);

    IHDRChunk ihdr;
    ihdr.Width = m_Width;
    ihdr.Height = m_Height;
    ihdr.BitDepth = bitDepth;
    ihdr.ColorType = colorType;
    ihdr.CompressionMethod = CompressionMethod::ZLIB;
    ihdr.FilterMethod = FilterMethod::ADAPTIVE_FILTERING;
    ihdr.InterlaceMethod = interlaceMethod;

    {
        Chunk chunk;
        PNG_RETURN_IF_NOT_OK(ihdr.Write, chunk);
        PNG_RETURN_IF_NOT_OK(chunk.Write, out);
        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    DynamicByteStream rawImage;
    Result rawReaderTRes;
    std::thread rawReaderT([this, &ihdr, &rawImage, &rawReaderTRes]() {
        rawReaderTRes = WriteRawPixels(ihdr.ColorType, ihdr.BitDepth, rawImage);
    });

    DynamicByteStream inf;
    Result interlacerTRes;
    std::thread interlacerT([&ihdr, samples, clevel, &rawImage, &inf, &interlacerTRes]() {
        interlacerTRes = InterlacePixels(ihdr.InterlaceMethod, ihdr.FilterMethod,
            ihdr.Width, ihdr.Height, ihdr.BitDepth, samples, clevel, rawImage, inf);
    });

    Result deflaterTRes;
    std::thread deflaterT([&out, &ihdr, clevel, &inf, &deflaterTRes]() {
        DynamicByteStream def;
        deflaterTRes = CompressData(ihdr.CompressionMethod, inf, def, clevel);
        if (deflaterTRes != Result::OK)
            return;

        Chunk chunk;
        chunk.Type = ChunkType::IDAT;
        chunk.Data = std::move(def.GetBuffer());
        chunk.CRC = chunk.CalculateCRC();

        deflaterTRes = chunk.Write(out);
        if (deflaterTRes != Result::OK)
            return;

        deflaterTRes = out.Flush();
        if (deflaterTRes != Result::OK)
            return;
    });

    rawReaderT.join();
    rawImage.Close();
    interlacerT.join();
    inf.Close();
    deflaterT.join();

    if (rawReaderTRes != Result::OK)
        return rawReaderTRes;
    if (interlacerTRes != Result::OK)
        return interlacerTRes;
    if (deflaterTRes != Result::OK)
        return deflaterTRes;

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
