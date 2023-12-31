#include "png/filter.h"

#include <iostream>
#include <memory>

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
            PNG_LDEBUGF("PNG::AdaptiveFiltering::UnfilterPixels Unknown filter type {} in image {}x{} (pb={},bpp={},sl={},y={}).",
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
                size_t pShift = 8 - (pixelStart & 7) - pixelBits; // pixelStart % 8
                size_t byte = (unf[y][pIndex] >> pShift) & pixelMask;
                out[y][x] = (uint8_t)byte;
            }
        }
    }

    return Result::OK;
}

PNG::Result PNG::AdaptiveFiltering::FilterPixels(size_t width, size_t height, size_t pixelBits, CompressionLevel clevel, IStream& in, OStream& out)
{
    // CompressionLevel::Default is the same as CompressionLevel::BestSpeed
    // CompressionLevel::BestSpeed uses a fixed filter
    // CompressionLevel::BestSize finds the best filter
    bool fixedFilter; // FilterType::PAETH
    bool optimize;
    switch (clevel) {
    case CompressionLevel::NoCompression:
        fixedFilter = false;
        optimize = false;
        break;
    case CompressionLevel::Default:
    case CompressionLevel::BestSpeed:
        fixedFilter = true;
        optimize = true;
        break;
    case CompressionLevel::BestSize:
        fixedFilter = false;
        optimize = true;
        break;
    default:
        PNG_UNREACHABLEF("PNG::AdaptiveFiltering::FilterPixels Missing case for CompressionLevel {}.", (int)clevel);
    }

    size_t bpp = BitsToBytes(pixelBits);
    size_t rowSize = width*bpp;

    std::vector<uint8_t> line(rowSize);

    size_t packedRowSize = BitsToBytes(width*pixelBits);
    // unf is valid only if optimize is true
    std::vector<uint8_t> _unf(optimize * packedRowSize * (height+1));
    ArrayView2D<uint8_t> unf(_unf.data(), 0, packedRowSize);

    // fil is used to calculate the current filtered line
    // The best-scoring one will be copied into line and then sent at the end
    // Scores are calculated following http://www.libpng.org/pub/png/spec/1.2/PNG-Encoders.html#E.Filter-selection
    uint8_t* fil = optimize ? unf[height] : nullptr;

    for (size_t y = 0; y < height; y++) {
        PNG_RETURN_IF_NOT_OK(in.ReadVector, line);

        // If pixels should be packed, pack them
        if (pixelBits < 8) {
            // This assert should never fail
            PNG_ASSERT(bpp == 1, "Packing a pixel which spans more than 1 byte.");
            const size_t pixelMask = (1 << pixelBits) - 1;
            for (size_t x = 0; x < width; x++) {
                uint8_t sample = line[x] & pixelMask;
                line[x] = 0;

                size_t pixelStart = pixelBits * x;
                size_t pIndex = pixelStart >> 3; // pixelStart / 8
                size_t pShift = 8 - (pixelStart & 7) - pixelBits; // pixelStart % 8
                line[pIndex] |= sample << pShift;
            }

            if (!optimize) {
                PNG_RETURN_IF_NOT_OK(out.WriteU8, 0);
                PNG_RETURN_IF_NOT_OK(out.WriteBuffer, line.data(), packedRowSize);
            }
        } else {
            PNG_ASSERT(packedRowSize == rowSize, "PNG::AdaptiveFiltering::FilterPixels Packed row check failed.");
            if (!optimize) {
                PNG_RETURN_IF_NOT_OK(out.WriteU8, 0);
                PNG_RETURN_IF_NOT_OK(out.WriteVector, line);
            }
        }

        if (optimize) {
            memcpy(unf[y], line.data(), packedRowSize);
            // line from this point contains the best scoring filter
            uint32_t bestFilter = FilterType::NONE;
            
            if (fixedFilter) {
                bestFilter = FilterType::PAETH;
                // FilterType::PAETH
                for (size_t i = 0; i < packedRowSize; i++) {
                    int32_t a = i >= bpp ? unf[y][i - bpp] : 0;
                    int32_t b = y > 0 ? unf[y-1][i] : 0;
                    int32_t c = y > 0 && i >= bpp ? unf[y-1][i - bpp] : 0;

                    int32_t p = a + b - c;
                    int32_t pa = abs(p-a);
                    int32_t pb = abs(p-b);
                    int32_t pc = abs(p-c);

                    line[i] = unf[y][i];
                    if (pa <= pb && pa <= pc)
                        line[i] -= a;
                    else if (pb <= pc)
                        line[i] -= b;
                    else
                        line[i] -= c;
                }
            } else {
                size_t lowestScore = 0;
                size_t currentScore;

                // FilterType::NONE
                for (size_t i = 0; i < packedRowSize; i++)
                    lowestScore += line[i];

                // FilterType::SUB
                currentScore = 0;
                for (size_t i = 0; i < packedRowSize; i++) {
                    uint8_t raw = i >= bpp ?
                        unf[y][i - bpp] : 0;
                    fil[i] = unf[y][i] - raw;
                    currentScore += fil[i];
                }

                if (currentScore < lowestScore) {
                    memcpy(line.data(), fil, packedRowSize);
                    bestFilter = FilterType::SUB;
                    lowestScore = currentScore;
                }

                // FilterType::UP
                currentScore = 0;
                for (size_t i = 0; i < packedRowSize; i++) {
                    uint8_t prior = y > 0 ?
                        unf[y-1][i] : 0;
                    fil[i] = unf[y][i] - prior;
                    currentScore += fil[i];
                }

                if (currentScore < lowestScore) {
                    memcpy(line.data(), fil, packedRowSize);
                    bestFilter = FilterType::UP;
                    lowestScore = currentScore;
                }

                // FilterType::AVERAGE
                currentScore = 0;
                for (size_t i = 0; i < packedRowSize; i++) {
                    uint32_t raw = i >= bpp ? unf[y][i - bpp] : 0;
                    uint32_t prior = y > 0 ? unf[y-1][i] : 0;
                    fil[i] += unf[y][i] - (raw + prior) / 2;
                    currentScore += fil[i];
                }

                if (currentScore < lowestScore) {
                    memcpy(line.data(), fil, packedRowSize);
                    bestFilter = FilterType::AVERAGE;
                    lowestScore = currentScore;
                }

                // FilterType::PAETH
                currentScore = 0;
                for (size_t i = 0; i < packedRowSize; i++) {
                    int32_t a = i >= bpp ? unf[y][i - bpp] : 0;
                    int32_t b = y > 0 ? unf[y-1][i] : 0;
                    int32_t c = y > 0 && i >= bpp ? unf[y-1][i - bpp] : 0;

                    int32_t p = a + b - c;
                    int32_t pa = abs(p-a);
                    int32_t pb = abs(p-b);
                    int32_t pc = abs(p-c);

                    fil[i] = unf[y][i];
                    if (pa <= pb && pa <= pc)
                        fil[i] -= a;
                    else if (pb <= pc)
                        fil[i] -= b;
                    else
                        fil[i] -= c;
                    currentScore += fil[i];
                }

                if (currentScore < lowestScore) {
                    memcpy(line.data(), fil, packedRowSize);
                    bestFilter = FilterType::PAETH;
                    lowestScore = currentScore;
                }
            }

            // Send filtered line
            PNG_RETURN_IF_NOT_OK(out.WriteU8, bestFilter);
            PNG_RETURN_IF_NOT_OK(out.WriteBuffer, line.data(), packedRowSize);
        }

        PNG_RETURN_IF_NOT_OK(out.Flush);
    }

    return Result::OK;
}

PNG::Result PNG::FilterPixels(uint8_t method, size_t width, size_t height, size_t pixelBits, CompressionLevel clevel, IStream& in, OStream& out)
{
    switch (method) {
    case FilterMethod::ADAPTIVE_FILTERING:
        return AdaptiveFiltering::FilterPixels(width, height, pixelBits, clevel, in, out);
    default:
        return Result::UnknownFilterMethod;
    }
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
