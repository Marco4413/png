#include "png/image.h"

#include "png/buffer.h"
#include "png/chunk.h"
#include "png/compression.h"
#include "png/interlace.h"

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

PNG::Result PNG::Image::Read(std::istream& in, PNG::Image& out)
{
    uint8_t sig[PNG::PNG_SIGNATURE_LEN];
    PNG_RETURN_IF_NOT_OK(PNG::ReadBuffer, in, sig, PNG::PNG_SIGNATURE_LEN);
    if (memcmp(sig, PNG::PNG_SIGNATURE, PNG::PNG_SIGNATURE_LEN) != 0)
        return PNG::Result::InvalidSignature;

    // Reading IHDR (Image Header)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    PNG_RETURN_IF_NOT_OK(PNG::Chunk::Read, in, chunk);
    PNG_RETURN_IF_NOT_OK(PNG::IHDRChunk::Parse, chunk, ihdr);

    // If color has 0 samples per component then it is not valid
    if (!ColorType::GetSamples(ihdr.ColorType))
        return Result::InvalidColorType;
    if (!ColorType::IsValidBitDepth(ihdr.ColorType, ihdr.BitDepth))
        return Result::InvalidBitDepth;

    // Vector holding all IDATs (Deflated Image Data)
    std::vector<uint8_t> idat;
    while (chunk.Type != PNG::ChunkType::IEND) {
        PNG_RETURN_IF_NOT_OK(PNG::Chunk::Read, in, chunk);
        bool isAux = PNG::ChunkType::IsAncillary(chunk.Type);

        switch (chunk.Type) {
        case PNG::ChunkType::IDAT: {
            size_t begin = idat.size();
            idat.resize(idat.size() + chunk.Length);
            memcpy(idat.data() + begin, chunk.Data, chunk.Length);
        }
        case PNG::ChunkType::IEND:
            break;
        default:
            // PLTE Chunk from the libpng standard isn't implemented yet.
            PNG_ASSERTF(isAux, "Mandatory chunk (%d) isn't supported yet.", chunk.Type);
        }
    }

    // Inflating IDAT
    std::vector<uint8_t> intPixels; // Interlaced Pixels
    PNG_RETURN_IF_NOT_OK(PNG::DecompressData, ihdr.CompressionMethod, idat, intPixels);

    PNG::ByteBuffer intPixelsBuf(intPixels);
    std::istream inIntPixels(&intPixelsBuf);

    size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    PNG_ASSERT(pixelSize > 0, "Pixel size is 0.");

    std::vector<uint8_t> rawPixels;
    PNG_RETURN_IF_NOT_OK(PNG::DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, inIntPixels, rawPixels);

    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, rawPixels);

    return Result::OK;
}
