#include "png/filter.h"

PNG::Result PNG::AdaptiveFiltering::UnfilterPixels(size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& _in, std::vector<uint8_t>& _out)
{
    int32_t rowSize = width*pixelSize;
    size_t outSize = width*height*pixelSize;

    const ArrayView2D<uint8_t> in(_in.data(), 1, rowSize + 1);
    if (_out.size() < outSize)
        _out.resize(outSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, rowSize);

    for (size_t y = 0; y < height; y++) {
        uint8_t filterType = in[y][-1];
        switch (filterType) {
        case FilterType::NONE:
            memcpy(out[y], in[y], rowSize);
            break;
        case FilterType::SUB:
            for (int32_t i = 0; i < rowSize; i++) {
                uint8_t raw = (i - (int32_t)pixelSize) >= 0 ?
                    out[y][i - pixelSize] : 0;
                out[y][i] = in[y][i] + raw;
            }
            break;
        case FilterType::UP:
            for (int32_t i = 0; i < rowSize; i++) {
                uint8_t prior = y > 0 ?
                    out[y-1][i] : 0;
                out[y][i] = in[y][i] + prior;
            }
            break;
        case FilterType::AVERAGE:
            for (int32_t i = 0; i < rowSize; i++) {
                uint32_t raw = (i - (int32_t)pixelSize) >= 0 ? out[y][i - pixelSize] : 0;
                uint32_t prior = y > 0 ? out[y-1][i] : 0;
                out[y][i] = (uint32_t)in[y][i] + (raw + prior) / 2;
            }
            break;
        case FilterType::PAETH:
            for (int32_t i = 0; i < rowSize; i++) {
                int32_t a = (i - (int32_t)pixelSize) >= 0 ? out[y][i - pixelSize] : 0;
                int32_t b = y > 0 ? out[y-1][i] : 0;
                int32_t c = y > 0 && (i - (int32_t)pixelSize) >= 0 ? out[y-1][i - pixelSize] : 0;

                int32_t p = a + b - c;
                int32_t pa = abs(p-a);
                int32_t pb = abs(p-b);
                int32_t pc = abs(p-c);

                out[y][i] = in[y][i];
                if (pa <= pb && pa <= pc)
                    out[y][i] += a;
                else if (pb <= pc)
                    out[y][i] += b;
                else
                    out[y][i] += c;
            }
            break;
        default:
            return Result::UnknownFilterType;
        }
    }

    _out.resize(outSize);
    return Result::OK;
}

PNG::Result PNG::UnfilterPixels(uint8_t method, size_t width, size_t height, size_t pixelSize, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case FilterMethod::ADAPTIVE_FILTERING:
        return AdaptiveFiltering::UnfilterPixels(width, height, pixelSize, in, out);
    default:
        return Result::UnknownFilterMethod;
    }
}
