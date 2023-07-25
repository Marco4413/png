#pragma once

#ifndef _PNG_INTERLACE_H
#define _PNG_INTERLACE_H

#include "png/base.h"
#include "png/stream.h"

namespace PNG
{
    namespace InterlaceMethod
    {
        const uint8_t NONE = 0;
        const uint8_t ADAM7 = 1;
    }

    namespace Adam7
    {
        Result DeinterlacePixels(uint8_t filterMethod, size_t width, size_t height,
            size_t bitDepth, size_t samples, IStream& in, std::vector<uint8_t>& out);
    }

    Result DeinterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height,
        size_t bitDepth, size_t samples, IStream& in, std::vector<uint8_t>& out);
}

#endif // _PNG_INTERLACE_H
