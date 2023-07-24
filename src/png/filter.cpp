#include "png/filter.h"

PNG::Result PNG::AdaptiveFiltering::UnfilterPixels(size_t width, size_t height, size_t pixelSize, IStream& in, std::vector<uint8_t>& _out)
{
    size_t rowSize = width*pixelSize;
    size_t outSize = width*height*pixelSize;

    std::vector<uint8_t> line;
    line.resize(rowSize + 1);

    if (_out.size() < outSize)
        _out.resize(outSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, rowSize);

    for (size_t y = 0; y < height; y++) {
        PNG_RETURN_IF_NOT_OK(in.ReadBuffer, line.data(), rowSize + 1);
        uint8_t filterType = line[0];
        switch (filterType) {
        case FilterType::NONE:
            memcpy(out[y], &line[1], rowSize);
            break;
        case FilterType::SUB:
            for (size_t i = 0; i < rowSize; i++) {
                uint8_t raw = i >= pixelSize ?
                    out[y][i - pixelSize] : 0;
                out[y][i] = line[i+1] + raw;
            }
            break;
        case FilterType::UP:
            for (size_t i = 0; i < rowSize; i++) {
                uint8_t prior = y > 0 ?
                    out[y-1][i] : 0;
                out[y][i] = line[i+1] + prior;
            }
            break;
        case FilterType::AVERAGE:
            for (size_t i = 0; i < rowSize; i++) {
                uint32_t raw = i >= pixelSize ? out[y][i - pixelSize] : 0;
                uint32_t prior = y > 0 ? out[y-1][i] : 0;
                out[y][i] = (uint32_t)line[i+1] + (raw + prior) / 2;
            }
            break;
        case FilterType::PAETH:
            for (size_t i = 0; i < rowSize; i++) {
                int32_t a = i >= pixelSize ? out[y][i - pixelSize] : 0;
                int32_t b = y > 0 ? out[y-1][i] : 0;
                int32_t c = y > 0 && i >= pixelSize ? out[y-1][i - pixelSize] : 0;

                int32_t p = a + b - c;
                int32_t pa = abs(p-a);
                int32_t pb = abs(p-b);
                int32_t pc = abs(p-c);

                out[y][i] = line[i+1];
                if (pa <= pb && pa <= pc)
                    out[y][i] += a;
                else if (pb <= pc)
                    out[y][i] += b;
                else
                    out[y][i] += c;
            }
            break;
        default:
            PNG_LDEBUGF("Unknown filter type %d in image %ldx%ld (ps=%ld,sl=%ld,y=%ld).", filterType, width, height, pixelSize, line.size(), y);
            return Result::UnknownFilterType;
        }
    }

    _out.resize(outSize);
    return Result::OK;
}

PNG::Result PNG::UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelSize, IStream& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case FilterMethod::ADAPTIVE_FILTERING:
        return AdaptiveFiltering::UnfilterPixels(width, height, pixelSize, in, out);
    default:
        return Result::UnknownFilterMethod;
    }
}
