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

        Result DecompressData(IStream& in, OStream& out);
    }
#endif // PNG_USE_ZLIB

    Result DecompressData(uint8_t method, IStream& in, OStream& out);
}

#endif // _PNG_COMPRESSION_H
