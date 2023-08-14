#pragma once

#ifndef _PNG_BASE_H
#define _PNG_BASE_H

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "fmt/color.h"

#define PNG_RETURN_IF_NOT_OK(func, ...) \
    do { \
        auto rres = (func)( __VA_ARGS__ ); \
        if (rres != Result::OK) \
            return rres; \
    } while (0)

#ifdef PNG_DEBUG

#define PNG_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fmt::print(stderr, fmt::fg(fmt::color::red), "{}:{}: Assert ({}) failed: {}\n", __FILE__, __LINE__, #cond, msg); \
            std::exit(1); \
        } \
    } while (0)

#define PNG_ASSERTF(cond, sfmt, ...) \
    do { \
        if (!(cond)) { \
            fmt::print(stderr, fmt::fg(fmt::color::red), "{}:{}: Assert ({}) failed: {}\n", __FILE__, __LINE__, #cond, \
                fmt::format(sfmt, __VA_ARGS__ )); \
            std::exit(1); \
        } \
    } while (0)

#define PNG_LDEBUG(msg) \
    fmt::print(stdout, fmt::fg(fmt::color::light_blue), "{}:{}: Debug: {}\n", __FILE__, __LINE__, msg)

#define PNG_LDEBUGF(sfmt, ...) \
    fmt::print(stdout, fmt::fg(fmt::color::light_blue), "{}:{}: Debug: {}\n", __FILE__, __LINE__, \
        fmt::format(sfmt, __VA_ARGS__ )); \

#else // PNG_DEBUG

#define PNG_ASSERT(cond, msg) do { (void)(cond); (void)(msg); } while (0)
#define PNG_ASSERTF(cond, fmt, ...) do { (void)(cond); (void)(fmt); } while (0)

#define PNG_LDEBUG(msg) do { (void)(msg); } while (0)
#define PNG_LDEBUGF(fmt, ...) do { (void)(fmt); } while (0)

#endif // PNG_DEBUG

#define PNG_UNREACHABLE(msg) \
    do { \
        fmt::print(stderr, fmt::fg(fmt::color::red), "{}:{}: Unreachable: {}\n", __FILE__, __LINE__, msg); \
        std::exit(1); \
    } while (0)

#define PNG_UNREACHABLEF(sfmt, ...) \
    do { \
        fmt::print(stderr, fmt::fg(fmt::color::red), "{}:{}: Unreachable: {}\n", __FILE__, __LINE__, \
            fmt::format(sfmt, __VA_ARGS__ )); \
        std::exit(1); \
    } while (0)

#define PNG_PI 3.141592653589793
#define PNG_E  2.718281828459045

namespace PNG
{
    const size_t PNG_SIGNATURE_LEN = 8;
    const uint8_t PNG_SIGNATURE[PNG_SIGNATURE_LEN] {137, 80, 78, 71, 13, 10, 26, 10};

    enum class Result
    {
        OK,
        EndOfFile,
        Unknown,
        UnexpectedEOF,
        UnexpectedChunkType,
        UnknownNecessaryChunk,
        UnknownCompressionMethod,
        UnknownFilterMethod,
        UnknownFilterType,
        UnknownInterlaceMethod,
        InvalidSignature,
        InvalidColorType,
        InvalidBitDepth,
        InvalidPixelBuffer,
        InvalidImageSize,
        UnsupportedColorType,
        CorruptedChunk,
        IllegalIHDRChunk,
        DuplicatePalette,
        IllegalPaletteChunk,
        InvalidPaletteSize,
        PaletteNotFound,
        InvalidPaletteIndex,
        IllegalIDATChunk,
        IllegalIDATSize,
        IllegaltRNSChunk,
        InvalidtRNSSize,
        InvalidTextualDataKeywordSize,
        UpdatingClosedStreamError,
        ZLib_NotAvailable,
        ZLib_DataError,
    };
    
    const char* ResultToString(Result res);

    template<typename T>
    class ArrayView2D
    {
    public:
        ArrayView2D(const T* data, size_t startOffset, size_t stride)
            : m_Data((T*)data), m_StartOffset(startOffset), m_Stride(stride) { }

        ArrayView2D(const ArrayView2D<T>& other) { *this = other; }
        
        ArrayView2D<T>& operator=(const ArrayView2D<T>& other)
        {
            m_Data = other.m_Data;
            m_StartOffset = other.m_StartOffset;
            m_Stride = other.m_Stride;
            return *this;
        }

        T* operator[](size_t y)
        {
            return &m_Data[m_StartOffset + m_Stride * y];
        }

        const T* operator[](size_t y) const
        {
            return &m_Data[m_StartOffset + m_Stride * y];
        }
    private:
        T* m_Data;
        size_t m_StartOffset;
        size_t m_Stride;
    };

    inline size_t BitsToBytes(size_t bits) { return bits & 7 ? (bits >> 3) + 1 : bits >> 3; }

    template<typename T>
    inline T Lerp(double t, const T& x0, const T& x1)
    {
        if (t < 0.0 || t > 1.0)
            PNG_LDEBUGF("PNG::Lerp t is not in the range [0; 1] {}", t);
        return (1.0 - t) * x0 + t * x1;
    }
} 

#endif // _PNG_BASE_H
