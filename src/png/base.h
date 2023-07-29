#pragma once

#ifndef _PNG_BASE_H
#define _PNG_BASE_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstring>
#include <vector>

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

#define PNG_LDEBUG(msg) \
        printf("%s:%d: Debug: %s\n", __FILE__, __LINE__, msg)

#define PNG_LDEBUGF(fmt, ...) \
    do { \
        printf("%s:%d: Debug: ", __FILE__, __LINE__); \
        printf(fmt, __VA_ARGS__ ); \
        printf("\n"); \
    } while (0)

#else // PNG_DEBUG

#define PNG_ASSERT(cond, msg) do { (void)(cond); (void)(msg); } while (0)
#define PNG_ASSERTF(cond, fmt, ...) do { (void)(cond); (void)(fmt); } while (0)

#define PNG_LDEBUG(msg) do { (void)(msg); } while (0)
#define PNG_LDEBUGF(fmt, ...) do { (void)(fmt); } while (0)

#endif // PNG_DEBUG

#define PNG_UNREACHABLE(msg) \
    do { \
        printf("%s:%d: Unreachable: %s\n", __FILE__, __LINE__, msg); \
        exit(1); \
    } while (0)

#define PNG_UNREACHABLEF(fmt, ...) \
    do { \
        printf("%s:%d: Unreachable: ", __FILE__, __LINE__); \
        printf(fmt, __VA_ARGS__ ); \
        printf("\n"); \
        exit(1); \
    } while (0)

namespace PNG
{
    const size_t PNG_SIGNATURE_LEN = 8;
    const uint8_t PNG_SIGNATURE[PNG_SIGNATURE_LEN] {137, 80, 78, 71, 13, 10, 26, 10};

    enum class Result
    {
        OK,
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
        DuplicatePalette,
        IllegalPaletteChunk,
        InvalidPaletteSize,
        PaletteNotFound,
        InvalidPaletteIndex,
        UpdatingClosedStreamError,
        ZLib_NotAvailable,
        ZLib_DataError,
        /* TODO: Custom ZLib implementation
        ZLib_InvalidCompressionMethod,
        ZLib_InvalidLZ77WindowSize,
        ZLib_FCHECKFailed,
        */
    };
    
    const char* ResultToString(Result res);

    template<typename T>
    class ArrayView2D
    {
    private:
        T* m_Data;
        size_t m_StartOffset;
        size_t m_Stride;
    public:
        ArrayView2D(T* data, size_t startOffset, size_t stride)
            : m_Data(data), m_StartOffset(startOffset), m_Stride(stride) { }
        ArrayView2D(const T* data, size_t startOffset, size_t stride)
            : ArrayView2D((T*)data, startOffset, stride) { }

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
    };

    inline size_t BitsToBytes(size_t bits) { return bits & 7 ? (bits >> 3) + 1 : bits >> 3; }
} 

#endif // _PNG_BASE_H
