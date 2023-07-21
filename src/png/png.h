#pragma once

#include <inttypes.h>

#include <istream>
#include <vector>

#ifndef _PNG_H
#define _PNG_H

#define PNG_USE_ZLIB
#define PNG_ENABLE_ASSERTS

#define PNG_RETURN_IF_NOT_OK(func, ...) \
    do { \
        auto rres = (func)( __VA_ARGS__ ); \
        if (rres != Result::OK) \
            return rres; \
    } while (0)

#ifdef PNG_ENABLE_ASSERTS

#define PNG_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("%s:%d: Assert (%s) failed: %s\n", __FILE__, __LINE__, #cond, msg); \
            exit(1); \
        } \
    } while (0)

#define PNG_ASSERTF(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            printf("%s:%d: Assert (%s) failed: ", __FILE__, __LINE__, #cond); \
            printf(fmt, __VA_ARGS__ ); \
            printf("\n"); \
            exit(1); \
        } \
    } while (0)

#else // PNG_ENABLE_ASSERTS

#define PNG_ASSERT(cond, msg)
#define PNG_ASSERTF(cond, fmt, ...)

#endif // PNG_ENABLE_ASSERTS

namespace PNG
{
    const static size_t PNG_SIGNATURE_LEN = 8;
    const static uint8_t PNG_SIGNATURE[PNG_SIGNATURE_LEN] {137, 80, 78, 71, 13, 10, 26, 10};

    enum class Result
    {
        OK,
        Unknown,
        UnexpectedEOF,
        UnexpectedChunkType,
        UnknownCompressionMethod,
        UnknownFilterMethod,
        UnknownFilterType,
        UnknownInterlaceMethod,
        InvalidSignature,
        InvalidColorType,
        InvalidBitDepth,
        InvalidPixelBuffer,
        InvalidImageSize,
        ZLib_NotAvailable,
        ZLib_DataError,
        /* TODO: Custom ZLib implementation
        ZLib_InvalidCompressionMethod,
        ZLib_InvalidLZ77WindowSize,
        ZLib_FCHECKFailed,
        */
    };
    
    const char* ResultToString(Result res);

    Result ReadBuffer(std::istream& input, void* buf, size_t bufLen);
    Result ReadUntilEOF(std::istream& input, std::vector<uint8_t>& out);

    template<typename T>
    Result ReadNumber(std::istream& input, T& out)
    {
        out = 0;
        uint8_t buf[1];
        for (size_t i = 0; i < sizeof(T); i++) {
            PNG_RETURN_IF_NOT_OK(ReadBuffer, input, buf, 1);
            out = (out << 8) | buf[0];
        }
        return Result::OK;
    }

    Result ReadU8(std::istream& input, uint8_t& out);
    Result ReadU32(std::istream& input, uint32_t& out);
    Result ReadU64(std::istream& input, uint64_t& out);

    class ByteBuffer : public std::streambuf
    {
    public:
        ByteBuffer(void* begin, void* end)
        {
            setg((char*)begin, (char*)begin, (char*)end);
        }

        ByteBuffer(void* buf, size_t bufLen)
            : ByteBuffer(buf, (char*)buf + bufLen) {}
    };

    /* TODO: CRC Implementation
    // Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
    class CRC
    {
    private:
        // This could be static
        uint32_t m_Table[256];
    public:
        CRC();
        uint32_t Update(uint32_t crc, void* buf, size_t bufLen) const;
        uint32_t Update(uint32_t crc, uint32_t val) const;
        uint32_t Calculate(void* buf, size_t bufLen) const;
    };
    */

    namespace ChunkType
    {
        inline bool IsAncillary(uint32_t chunkType)  { return (chunkType & 0x20000000) != 0; }
        inline bool IsPrivate(uint32_t chunkType)    { return (chunkType & 0x00200000) != 0; }
        inline bool IsConforming(uint32_t chunkType) { return (chunkType & 0x00002000) != 0; }
        inline bool IsSafeToCopy(uint32_t chunkType) { return (chunkType & 0x00000020) != 0; }

        const uint32_t IHDR = 0x49484452;
        const uint32_t IDAT = 0x49444154;
        const uint32_t IEND = 0x49454e44;
    }

    class Chunk
    {
    public:
        uint32_t Length = 0;
        uint32_t Type;
        uint8_t* Data = nullptr;
        uint32_t CRC;
    public:
        Chunk() = default;
        ~Chunk() { delete[] Data; }

        /* TODO: CRC Implementation
        uint32_t CalculateCRC(const PNG::CRC& crcDevice) const;
        */
        
        static Result Read(std::istream& input, Chunk& chunk);
    };

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

    class IHDRChunk
    {
    public:
        uint32_t Width;
        uint32_t Height;
        uint8_t BitDepth;
        uint8_t ColorType;
        uint8_t CompressionMethod;
        uint8_t FilterMethod;
        uint8_t InterlaceMethod;
    
        static Result Parse(const Chunk& chunk, IHDRChunk& ihdr);
    };

    namespace CompressionMethod
    {
        const uint8_t ZLIB = 0;
    }

#ifdef PNG_USE_ZLIB
    namespace ZLib
    {
        /* TODO: Custom ZLib implementation
        namespace CompressionLevel
        {
            const uint8_t Fastest = 0;
            const uint8_t Fast = 1;
            const uint8_t Default = 2;
            const uint8_t Slowest = 3;
        }*/

        Result DecompressData(const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
    }
#endif // PNG_USE_ZLIB

    Result DecompressData(uint8_t method, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

    namespace FilterMethod
    {
        const uint8_t ADAPTIVE_FILTERING = 0;
    }

    template<typename T>
    class ArrayView2D
    {
    private:
        T* m_Data;
        const size_t m_StartOffset;
        const size_t m_Stride;
    public:
        ArrayView2D(T* data, size_t startOffset, size_t stride)
            : m_Data(data), m_StartOffset(startOffset), m_Stride(stride) { }
        ArrayView2D(const T* data, size_t startOffset, size_t stride)
            : ArrayView2D((T*)data, startOffset, stride) { }

        T* operator[](size_t y)
        {
            return &m_Data[m_StartOffset + m_Stride * y];
        }

        const T* operator[](size_t y) const
        {
            return &m_Data[m_StartOffset + m_Stride * y];
        }
    };

    namespace AdaptiveFiltering
    {
        namespace FilterType
        {
            const uint8_t NONE = 0;
            const uint8_t SUB = 1;
            const uint8_t UP = 2;
            const uint8_t AVERAGE = 3;
            const uint8_t PAETH = 4;
        }

        Result UnfilterPixels(size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);
    }

    Result UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& in, std::vector<uint8_t>& out);

    namespace InterlaceMethod
    {
        const uint8_t NONE = 0;
        const uint8_t ADAM7 = 1;
    }

    namespace Adam7
    {
        Result DeinterlacePixels(uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& out);
    }

    Result DeinterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& out);

    struct Color
    {
        float R, G, B, A;
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
        
        Result LoadRawPixels(size_t samples, size_t bitDepth, std::vector<uint8_t>& in);
        static Result Read(std::istream& in, Image& out);
    };
}

#endif // _PNG_H
