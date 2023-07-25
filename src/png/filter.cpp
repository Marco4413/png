#include "png/filter.h"

#include <iostream>

PNG::Result PNG::AdaptiveFiltering::UnfilterPixels(size_t width, size_t height, size_t pixelBits, IStream& in, std::vector<uint8_t>& _out)
{
    size_t bpp = BitsToBytes(pixelBits);
    size_t rowSize = width*bpp;
    size_t outSize = rowSize*height;

    size_t packedRowSize = BitsToBytes(width*pixelBits);
    size_t unfSize = packedRowSize*height;
    std::vector<uint8_t> unfiltered;
    unfiltered.resize(unfSize);

    if (_out.size() != outSize)
        _out.resize(outSize);

    ArrayView2D<uint8_t> unf(unfiltered.data(), 0, packedRowSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, rowSize);
    if (pixelBits >= 8)
        unf = out;

    for (size_t y = 0; y < height; y++) {
        uint8_t filterType;
        PNG_RETURN_IF_NOT_OK(in.ReadU8, filterType);
        PNG_RETURN_IF_NOT_OK(in.ReadBuffer, unf[y], packedRowSize);
        
        // for (size_t i = 0; i < line.size(); i++)
        //     std::cout << (size_t)line[i] << ", ";
        // std::cout << std::endl;

        switch (filterType) {
        case FilterType::NONE:
            // If pixels are not packed unf == out
            // Otherwise out is filled a the end of this switch
            // memcpy(unf[y], &line[1], rowSize);
            break;
        case FilterType::SUB:
            for (size_t i = 0; i < packedRowSize; i++) {
                uint8_t raw = i >= bpp ?
                    unf[y][i - bpp] : 0;
                unf[y][i] += raw;
            }
            break;
        case FilterType::UP:
            for (size_t i = 0; i < packedRowSize; i++) {
                uint8_t prior = y > 0 ?
                    unf[y-1][i] : 0;
                unf[y][i] += prior;
            }
            break;
        case FilterType::AVERAGE:
            for (size_t i = 0; i < packedRowSize; i++) {
                uint32_t raw = i >= bpp ? unf[y][i - bpp] : 0;
                uint32_t prior = y > 0 ? unf[y-1][i] : 0;
                unf[y][i] += (raw + prior) / 2;
            }
            break;
        case FilterType::PAETH:
            for (size_t i = 0; i < packedRowSize; i++) {
                int32_t a = i >= bpp ? unf[y][i - bpp] : 0;
                int32_t b = y > 0 ? unf[y-1][i] : 0;
                int32_t c = y > 0 && i >= bpp ? unf[y-1][i - bpp] : 0;

                int32_t p = a + b - c;
                int32_t pa = abs(p-a);
                int32_t pb = abs(p-b);
                int32_t pc = abs(p-c);

                if (pa <= pb && pa <= pc)
                    unf[y][i] += a;
                else if (pb <= pc)
                    unf[y][i] += b;
                else
                    unf[y][i] += c;
            }
            break;
        default:
            PNG_LDEBUGF("Unknown filter type %d in image %ldx%ld (pb=%ld,bpp=%ld,sl=%ld,y=%ld).",
                filterType, width, height, pixelBits, bpp, packedRowSize+1, y);
            return Result::UnknownFilterType;
        }

        // If pixels are packed unpack them
        if (pixelBits < 8) {
            // This assert should never fail
            PNG_ASSERT(bpp == 1, "Unpacking a pixel which spans more than 1 byte.");
            // Here unf != out
            const size_t pixelMask = (1 << pixelBits) - 1;
            for (size_t x = 0; x < width; x++) {
                size_t pixelStart = pixelBits * x;
                size_t pIndex = pixelStart >> 3; // pixelStart / 8
                size_t pShift = 8 - (pixelStart & 7) + pixelBits; // pixelStart % 8
                size_t byte = (unf[y][pIndex] >> pShift) & pixelMask;
                out[y][x] = byte;
            }
        }
    }

    return Result::OK;
}

PNG::Result PNG::UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelBits, IStream& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case FilterMethod::ADAPTIVE_FILTERING:
        return AdaptiveFiltering::UnfilterPixels(width, height, pixelBits, in, out);
    default:
        return Result::UnknownFilterMethod;
    }
}
