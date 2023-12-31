#pragma once

#ifndef _PNG_FILTER_H
#define _PNG_FILTER_H

#include "png/base.h"
#include "png/compression.h"
#include "png/stream.h"

namespace PNG
{
    namespace FilterMethod
    {
        const uint8_t ADAPTIVE_FILTERING = 0;
    }

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

        Result FilterPixels(size_t width, size_t height, size_t pixelBits, CompressionLevel clevel, IStream& in, OStream& out);
        Result UnfilterPixels(size_t width, size_t height, size_t pixelBits, IStream& in, std::vector<uint8_t>& out);
    }

    Result FilterPixels(uint8_t method, size_t width, size_t height, size_t pixelBits, CompressionLevel clevel, IStream& in, OStream& out);
    Result UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelBits, IStream& in, std::vector<uint8_t>& out);
}

#endif // _PNG_FILTER_H
