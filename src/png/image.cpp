#include "png/image.h"

#include "png/chunk.h"
#include "png/compression.h"
#include "png/interlace.h"

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

PNG::Result PNG::Image::LoadRawPixels(size_t colorType, size_t bitDepth, std::vector<uint8_t>& in)
{
    PNG_ASSERT(colorType != ColorType::PALETTE, "Palette color type not supported.");

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
        case ColorType::PALETTE:
        default:
            return Result::InvalidColorType;
        }

        m_Pixels[pixelI++] = color;
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

    if (ihdr.Width == 0 || ihdr.Height == 0)
        return Result::InvalidImageSize;

    // If color has 0 samples per component then it is not valid
    if (!ColorType::GetSamples(ihdr.ColorType))
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    // Vector holding all IDATs (Deflated Image Data)
    
    DynamicByteStream idat(std::chrono::milliseconds(0));
    do {
        PNG_RETURN_IF_NOT_OK(Chunk::Read, in, chunk);
        bool isAux = ChunkType::IsAncillary(chunk.Type);

        switch (chunk.Type) {
        case ChunkType::IDAT:
            idat.WriteBuffer(chunk.Data, chunk.Length);
            idat.Flush();
        case ChunkType::IEND:
            break;
        default:
            // PLTE Chunk from the libpng standard isn't implemented yet.
            PNG_ASSERTF(isAux, "Mandatory chunk (%d) isn't supported yet.", chunk.Type);
        }
    } while (chunk.Type != ChunkType::IEND);

    DynamicByteStream intPixels(std::chrono::milliseconds(0)); // Interlaced Pixels

    // Inflating IDAT
    PNG_RETURN_IF_NOT_OK(DecompressData, ihdr.CompressionMethod, idat, intPixels);

    std::vector<uint8_t> rawPixels;

    size_t pixelSize = ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    PNG_ASSERT(pixelSize > 0, "Pixel size is 0.");

    PNG_RETURN_IF_NOT_OK(DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod,
        ihdr.Width, ihdr.Height, pixelSize, intPixels, rawPixels);

    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, rawPixels);

    return Result::OK;
}

PNG::Result PNG::Image::ReadMT(IStream& in, PNG::Image& out, std::chrono::milliseconds timeout)
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

    if (ihdr.Width == 0 || ihdr.Height == 0)
        return Result::InvalidImageSize;

    // If color has 0 samples per component then it is not valid
    if (!ColorType::GetSamples(ihdr.ColorType))
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    // Vector holding all IDATs (Deflated Image Data)
    
    DynamicByteStream idat(timeout);
    Result readerTRes;


    std::thread readerT([&in, &idat, &readerTRes]() {
        size_t idats = 0;

        Chunk chunk;
        do {
            readerTRes = Chunk::Read(in, chunk);
            if (readerTRes != Result::OK)
                return;

            bool isAux = ChunkType::IsAncillary(chunk.Type);

            switch (chunk.Type) {
            case ChunkType::IDAT:
                idats++;
                idat.WriteBuffer(chunk.Data, chunk.Length);
                idat.Flush();
            case ChunkType::IEND:
                break;
            default:
                // PLTE Chunk from the libpng standard isn't implemented yet.
                PNG_ASSERTF(isAux, "Mandatory chunk (%d) isn't supported yet.", chunk.Type);
            }
        } while (chunk.Type != ChunkType::IEND);
        // While reading IDATs, PNG::DecompressData can read the buffer in another thread
    });

    DynamicByteStream intPixels(timeout); // Interlaced Pixels
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
        size_t pixelSize = ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
        PNG_ASSERT(pixelSize > 0, "Pixel size is 0.");

        deinterlacerTRes = DeinterlacePixels(ihdr.InterlaceMethod, ihdr.FilterMethod,
            ihdr.Width, ihdr.Height, pixelSize, intPixels, rawPixels);
    });

    PNG_LDEBUG("Joining IDAT Reader.");
    readerT.join();
    PNG_LDEBUG("Joining IDAT Inflater.");
    inflaterT.join();
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
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, rawPixels);

    return Result::OK;
}
