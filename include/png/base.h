#pragma once

#ifndef _PNG_BASE_H
#define _PNG_BASE_H

#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <vector>

#ifdef PNG_NO_LOGGING
#define PNG_PRINTF(...) (void)(0)
#else // PNG_NO_LOGGING
#include "fmt/color.h"

#define PNG_PRINTF fmt::print
#endif // PNG_NO_LOGGING

#define PNG_RETURN_IF_NOT_OK(func, ...) \
    do { \
        auto rres = (func)( __VA_ARGS__ ); \
        if (rres != Result::OK) \
            return rres; \
    } while (0)

#ifdef PNG_DEBUG

// PNG_DEBUG_BREAK
#if defined(_MSC_VER)
  // MSC has a __debugbreak intrinsic that can be called for breakpoints
  #include <intrin.h>
  #define PNG_DEBUG_BREAK() __debugbreak()
// PNG_DEBUG_BREAK
#elif defined(SIGTRAP)
  // POSIX systems SHOULD have the SIGTRAP signal for debug breakpoints
  #define PNG_DEBUG_BREAK() std::raise(SIGTRAP)
#else // PNG_DEBUG_BREAK
  #define PNG_DEBUG_BREAK() (void)(0)
#endif // PNG_DEBUG_BREAK

#define PNG_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            PNG_PRINTF(stderr, fmt::fg(fmt::color::red), "{}:{}: Assert ({}) failed: {}\n", __FILE__, __LINE__, #cond, msg); \
            PNG_DEBUG_BREAK(); \
            std::exit(1); \
        } \
    } while (0)

#define PNG_ASSERTF(cond, sfmt, ...) \
    do { \
        if (!(cond)) { \
            PNG_PRINTF(stderr, fmt::fg(fmt::color::red), "{}:{}: Assert ({}) failed: {}\n", __FILE__, __LINE__, #cond, \
                fmt::format(sfmt, __VA_ARGS__ )); \
            PNG_DEBUG_BREAK(); \
            std::exit(1); \
        } \
    } while (0)

#define PNG_LDEBUG(msg) \
    PNG_PRINTF(stdout, fmt::fg(fmt::color::light_blue), "{}:{}: Debug: {}\n", __FILE__, __LINE__, msg)

#define PNG_LDEBUGF(sfmt, ...) \
    PNG_PRINTF(stdout, fmt::fg(fmt::color::light_blue), "{}:{}: Debug: {}\n", __FILE__, __LINE__, \
        fmt::format(sfmt, __VA_ARGS__ )); \

#else // PNG_DEBUG

#define PNG_DEBUG_BREAK() (void)(0)

#define PNG_ASSERT(cond, msg) do { (void)(cond); (void)(msg); } while (0)
#define PNG_ASSERTF(cond, fmt, ...) do { (void)(cond); (void)(fmt); } while (0)

#define PNG_LDEBUG(msg) (void)(msg)
#define PNG_LDEBUGF(fmt, ...) (void)(fmt)

#endif // PNG_DEBUG

#define PNG_UNREACHABLE(msg) \
    do { \
        PNG_PRINTF(stderr, fmt::fg(fmt::color::red), "{}:{}: Unreachable: {}\n", __FILE__, __LINE__, msg); \
        PNG_DEBUG_BREAK(); \
        std::exit(1); \
    } while (0)

#define PNG_UNREACHABLEF(sfmt, ...) \
    do { \
        PNG_PRINTF(stderr, fmt::fg(fmt::color::red), "{}:{}: Unreachable: {}\n", __FILE__, __LINE__, \
            fmt::format(sfmt, __VA_ARGS__ )); \
        PNG_DEBUG_BREAK(); \
        std::exit(1); \
    } while (0)

#define PNG_PI 3.141592653589793
#define PNG_E  2.718281828459045

namespace PNG
{
    constexpr size_t PNG_SIGNATURE_LEN = 8;
    constexpr uint8_t PNG_SIGNATURE[PNG_SIGNATURE_LEN] {137, 80, 78, 71, 13, 10, 26, 10};

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
        IllegaltIMEChunk,
        InvalidLMTMonth,
        InvalidLMTDay,
        InvalidLMTHour,
        InvalidLMTMinute,
        InvalidLMTSecond,
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
