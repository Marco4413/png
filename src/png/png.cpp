#include "png/png.h"

#include <cstring>

#ifdef PNG_USE_ZLIB
#include <zlib/zlib.h>
#endif

const char* PNG::ResultToString(PNG::Result res)
{
    switch (res) {
    case Result::OK:
        return "OK";
    case Result::Unknown:
        return "Unknown";
    case Result::UnexpectedEOF:
        return "UnexpectedEOF";
    case Result::UnexpectedChunkType:
        return "UnexpectedChunkType";
    case Result::UnknownCompressionMethod:
        return "UnknownCompressionMethod";
    case Result::UnknownFilterMethod:
        return "UnknownFilterMethod";
    case Result::UnknownFilterType:
        return "UnknownFilterType";
    case Result::UnknownInterlaceMethod:
        return "UnknownInterlaceMethod";
    case Result::InvalidSignature:
        return "InvalidSignature";
    case Result::InvalidColorType:
        return "InvalidColorType";
    case Result::InvalidBitDepth:
        return "InvalidBitDepth";
    case Result::InvalidPixelBuffer:
        return "InvalidPixelBuffer";
    case Result::InvalidImageSize:
        return "InvalidImageSize";
    /* TODO: Custom ZLib implementation
    case Result::ZLib_InvalidCompressionMethod:
        return "ZLib_InvalidCompressionMethod";
    case Result::ZLib_InvalidLZ77WindowSize:
        return "ZLib_InvalidLZ77WindowSize";
    */
    case Result::ZLib_NotAvailable:
        return "ZLib_NotAvailable";
    case Result::ZLib_DataError:
        return "ZLib_DataError";
    default:
        PNG_ASSERTF(false, "Missing %d code.", (int)res);
    }
}

PNG::Result PNG::ReadBuffer(std::istream& input, void* buf, size_t bufLen)
{
    if (!input.read((char*)buf, bufLen)) {
        if (input.eof())
            return Result::UnexpectedEOF;
        return Result::Unknown;
    }
    return Result::OK;
}


PNG::Result PNG::ReadUntilEOF(std::istream& input, std::vector<uint8_t>& out)
{
    char ch;
    while (input.read(&ch, 1))
        out.push_back(ch);
    if (input.eof())
        return Result::OK;
    return Result::Unknown;
}

inline PNG::Result PNG::ReadU8(std::istream& input, uint8_t& out)
{
    return ReadBuffer(input, &out, 1);
}

inline PNG::Result PNG::ReadU32(std::istream& input, uint32_t& out)
{
    return ReadNumber(input, out);
}

inline PNG::Result PNG::ReadU64(std::istream& input, uint64_t& out)
{
    return ReadNumber(input, out);
}

/* TODO: CRC Implementation
// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
PNG::CRC::CRC()
{
    for (size_t i = 0; i < 256; i++) {
        uint32_t val = i;
        for (size_t k = 0; k < 8; k++) {
            if ((val & 1) != 0)
                val = 0xedb88320L ^ (val >> 1);
            else
                val >>= 1;
        }
        m_Table[i] = val;
    }
}

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
uint32_t PNG::CRC::Update(uint32_t crc, void* _buf, size_t bufLen) const
{
    uint8_t* buf = (uint8_t*)_buf;
    for (size_t i = 0; i < bufLen; i++)
        crc = m_Table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    return crc;
}

uint32_t PNG::CRC::Update(uint32_t crc, uint32_t value) const
{
    for (int i = 3; i >= 0; i--) {
        uint8_t byte = (uint8_t)(value >> (i * 8));
        crc = Update(crc, &byte, 1);
    }
    return crc;
}

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
inline uint32_t PNG::CRC::Calculate(void* _buf, size_t bufLen) const
{
    return ~Update(~0, _buf, bufLen);
}

uint32_t PNG::Chunk::CalculateCRC(const PNG::CRC& crcDevice) const
{
    uint32_t crc = ~0;
    crc = crcDevice.Update(crc, Length);
    crc = crcDevice.Update(crc, Type);
    crc = crcDevice.Update(crc, Data, Length);
    return ~crc;
}
*/

PNG::Result PNG::Chunk::Read(std::istream& input, PNG::Chunk& chunk)
{
    if (chunk.Data) {
        delete[] chunk.Data;
        chunk.Data = nullptr;
    }
    chunk.Length = 0;

    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.Length);
    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.Type);
    chunk.Data = new uint8_t[chunk.Length];
    PNG_RETURN_IF_NOT_OK(ReadBuffer, input, chunk.Data, chunk.Length);
    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.CRC);

    return Result::OK;
}

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

PNG::Result PNG::IHDRChunk::Parse(const PNG::Chunk& chunk, PNG::IHDRChunk& ihdr)
{
    if (chunk.Type != ChunkType::IHDR)
        return Result::UnexpectedChunkType;
    
    ByteBuffer ihdrBuf(chunk.Data, chunk.Length);
    std::istream inIHDR(&ihdrBuf);

    PNG_RETURN_IF_NOT_OK(ReadU32, inIHDR, ihdr.Width);
    PNG_RETURN_IF_NOT_OK(ReadU32, inIHDR, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.BitDepth);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.ColorType);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.CompressionMethod);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.FilterMethod);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.InterlaceMethod);

    return Result::OK;
}

#ifdef PNG_USE_ZLIB
PNG::Result PNG::ZLib::DecompressData(const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    const size_t K32 = 32768; // 32K of data
    out.resize(K32);

    z_stream inf;
    inf.zalloc = Z_NULL;
    inf.zfree = Z_NULL;
    inf.opaque = Z_NULL;
    inf.avail_in = in.size();
    inf.next_in = (uint8_t*)in.data();
    inf.avail_out = out.size();
    inf.next_out = out.data();

    inflateInit(&inf);
    int zcode;
    do {
        zcode = inflate(&inf, Z_NO_FLUSH);
        if (inf.avail_out == 0) {
            out.resize(out.size() + K32);
            inf.avail_out = K32;
            inf.next_out = out.data() + out.size() - K32;
        }
    } while (zcode == Z_OK);
    inflateEnd(&inf);

    if (zcode == Z_STREAM_END) {
        out.resize(out.size() - inf.avail_out);
        return Result::OK;
    }

    return Result::ZLib_DataError;

    /* TODO: Custom ZLib implementation
    // ZLib RFC-1950 http://www.zlib.org/rfc1950.pdf
    uint8_t CMF, FLG;
    PNG_RETURN_IF_NOT_OK(ReadU8, in, CMF);
    PNG_RETURN_IF_NOT_OK(ReadU8, in, FLG);

    // The first 4 bits of CMF indicate the Compression Method (CM) used
    // PNG only supports CM = 8
    if ((CMF & 0x0f) != 8)
        return Result::ZLib_InvalidCompressionMethod;
    
    // The last 4 bits of CMF indicate Compression Info (CINFO) used
    // CINFO is log2(windowSize) - 8. PNG supports a window size up to 32K
    size_t windowSize = 1 << ((CMF >> 4) + 8);
    if (windowSize > 32768)
        return Result::ZLib_InvalidLZ77WindowSize;
    
    { // FCHECK
        uint16_t CMF_FLG = (((uint16_t)CMF) << 8) + FLG;
        if (CMF_FLG % 31 != 0)
            return Result::ZLib_FCHECKFailed;
    }

    bool FDICT = FLG & 0x20;
    PNG_ASSERT(!FDICT, "PNG::DecompressZLibData FDICT not supported.");
    std::cout << (windowSize) << std::endl;

    // DEFLATE RFC-1951 https://www.rfc-editor.org/rfc/pdfrfc/rfc1951.txt.pdf
    */
}
#endif // PNG_USE_ZLIB

PNG::Result PNG::DecompressData(uint8_t method, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case CompressionMethod::ZLIB:
#ifdef PNG_USE_ZLIB
        return ZLib::DecompressData(in, out);
#else // PNG_USE_ZLIB
        (void) in;
        (void) out;
        return Result::ZLib_NotAvailable;
#endif // PNG_USE_ZLIB
    default:
        return Result::UnknownCompressionMethod;
    }
}

PNG::Result PNG::AdaptiveFiltering::UnfilterPixels(size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& _in, std::vector<uint8_t>& _out)
{
    int32_t rowSize = width*pixelSize;
    size_t outSize = width*height*pixelSize;

    const ArrayView2D<uint8_t> in(_in.data(), 1, rowSize + 1);
    if (_out.size() < outSize)
        _out.resize(outSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, rowSize);

    for (size_t y = 0; y < height; y++) {
        uint8_t filterType = in[y][-1];
        switch (filterType) {
        case FilterType::NONE:
            memcpy(out[y], in[y], rowSize);
            break;
        case FilterType::SUB:
            for (int32_t i = 0; i < rowSize; i++) {
                uint8_t raw = (i - (int32_t)pixelSize) >= 0 ?
                    out[y][i - pixelSize] : 0;
                out[y][i] = in[y][i] + raw;
            }
            break;
        case FilterType::UP:
            for (int32_t i = 0; i < rowSize; i++) {
                uint8_t prior = y > 0 ?
                    out[y-1][i] : 0;
                out[y][i] = in[y][i] + prior;
            }
            break;
        case FilterType::AVERAGE:
            for (int32_t i = 0; i < rowSize; i++) {
                uint32_t raw = (i - (int32_t)pixelSize) >= 0 ? out[y][i - pixelSize] : 0;
                uint32_t prior = y > 0 ? out[y-1][i] : 0;
                out[y][i] = (uint32_t)in[y][i] + (raw + prior) / 2;
            }
            break;
        case FilterType::PAETH:
            for (int32_t i = 0; i < rowSize; i++) {
                int32_t a = (i - (int32_t)pixelSize) >= 0 ? out[y][i - pixelSize] : 0;
                int32_t b = y > 0 ? out[y-1][i] : 0;
                int32_t c = y > 0 && (i - (int32_t)pixelSize) >= 0 ? out[y-1][i - pixelSize] : 0;

                int32_t p = a + b - c;
                int32_t pa = abs(p-a);
                int32_t pb = abs(p-b);
                int32_t pc = abs(p-c);

                out[y][i] = in[y][i];
                if (pa <= pb && pa <= pc)
                    out[y][i] += a;
                else if (pb <= pc)
                    out[y][i] += b;
                else
                    out[y][i] += c;
            }
            break;
        default:
            return Result::UnknownFilterType;
        }
    }

    _out.resize(outSize);
    return Result::OK;
}

PNG::Result PNG::UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case FilterMethod::ADAPTIVE_FILTERING:
        return AdaptiveFiltering::UnfilterPixels(width, height, pixelSize, in, out);
    default:
        return Result::UnknownFilterMethod;
    }
}

PNG::Result PNG::Adam7::DeinterlacePixels(uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& _out)
{
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Decoders.html#D.Progressive-display
    const size_t STARTING_COL[7] { 0, 4, 0, 2, 0, 1, 0 };
    const size_t STARTING_ROW[7] { 0, 0, 4, 0, 2, 0, 1 };
    const size_t COL_OFFSET[7]   { 8, 8, 4, 4, 2, 2, 1 };
    const size_t ROW_OFFSET[7]   { 8, 8, 8, 4, 4, 2, 2 };

    _out.resize(width*height*pixelSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, width*pixelSize);

    std::vector<uint8_t> passImage;
    for (size_t pass = 0; pass < 7; pass++) {
        size_t passWidth  = width  / COL_OFFSET[pass];
        size_t passHeight = height / ROW_OFFSET[pass];
        passImage.resize(passWidth*passHeight*pixelSize+passHeight);
        PNG_RETURN_IF_NOT_OK(ReadBuffer, in, passImage.data(), passImage.size());
        PNG_RETURN_IF_NOT_OK(UnfilterPixels, filterMethod, passWidth, passHeight, pixelSize, passImage, passImage);
        for (size_t py = 0; py < passHeight; py++) {
            for (size_t px = 0; px < passWidth; px++) {
                size_t outY = STARTING_ROW[pass] + py * ROW_OFFSET[pass];
                size_t outI = (STARTING_COL[pass] + px * COL_OFFSET[pass]) * pixelSize;
                size_t passI = (py*passWidth+px) * pixelSize;
                memcpy(&out[outY][outI], &passImage[passI], pixelSize);
            }
        }
    }

    return Result::OK;
}

PNG::Result PNG::DeinterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& out)
{
    out.resize(0);
    switch (method) {
    case InterlaceMethod::NONE:
        out.reserve(width*height*pixelSize+height);
        PNG_RETURN_IF_NOT_OK(ReadUntilEOF, in, out);
        return UnfilterPixels(filterMethod, width, height, pixelSize, out, out);
    case InterlaceMethod::ADAM7:
        return Adam7::DeinterlacePixels(filterMethod, width, height, pixelSize, in, out);
    default:
        return Result::UnknownInterlaceMethod;
    }
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

    PNG::ByteBuffer intPixelsBuf(intPixels.data(), intPixels.size());
    std::istream inIntPixels(&intPixelsBuf);

    size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    PNG_ASSERT(pixelSize > 0, "Pixel size is 0.");

    std::vector<uint8_t> rawPixels;
    PNG_RETURN_IF_NOT_OK(PNG::DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, inIntPixels, rawPixels);

    out.SetSize(ihdr.Width, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(out.LoadRawPixels, ihdr.ColorType, ihdr.BitDepth, rawPixels);

    return Result::OK;
}
