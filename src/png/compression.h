#pragma once

#ifndef _PNG_COMPRESSION_H
#define _PNG_COMPRESSION_H

#include "png/base.h"
#include "png/stream.h"

#define PNG_USE_ZLIB

namespace PNG
{
    namespace CompressionMethod
    {
        const uint8_t ZLIB = 0;
    }

    enum class CompressionLevel
    {
        Default,
        NoCompression,
        BestSpeed,
        BestSize,
    };

#ifdef PNG_USE_ZLIB
    namespace ZLib
    {
        int GetLevel(CompressionLevel l);

        Result DecompressData(IStream& in, OStream& out);
        Result CompressData(IStream& in, OStream& out, CompressionLevel level = CompressionLevel::Default);
    }
#endif // PNG_USE_ZLIB

    Result DecompressData(uint8_t method, IStream& in, OStream& out);
    Result CompressData(uint8_t method, IStream& in, OStream& out, CompressionLevel level = CompressionLevel::Default);
}

#endif // _PNG_COMPRESSION_H
