#include "png/interlace.h"

#include "png/buffer.h"
#include "png/filter.h"

PNG::Result PNG::Adam7::DeinterlacePixels(uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& _out)
{
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Decoders.html#D.Progressive-display
    const size_t STARTING_COL[7] { 0, 4, 0, 2, 0, 1, 0 };
    const size_t STARTING_ROW[7] { 0, 0, 4, 0, 2, 0, 1 };
    const size_t COL_OFFSET[7]   { 8, 8, 4, 4, 2, 2, 1 };
    const size_t ROW_OFFSET[7]   { 8, 8, 8, 4, 4, 2, 2 };

    _out.resize(width*height*pixelSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, width*pixelSize);

    std::vector<uint8_t> passImage;
    for (size_t pass = 0; pass < 7; pass++) {
        size_t passWidth  = width  / COL_OFFSET[pass];
        size_t passHeight = height / ROW_OFFSET[pass];
        
        passImage.resize(passWidth*passHeight*pixelSize+passHeight);
        ByteBuffer passBuf(passImage.data(), passImage.size());
        std::istream inPass(&passBuf);

        PNG_RETURN_IF_NOT_OK(ReadBuffer, in, passImage.data(), passImage.size());
        PNG_RETURN_IF_NOT_OK(UnfilterPixels, filterMethod, passWidth, passHeight, pixelSize, inPass, passImage);
        for (size_t py = 0; py < passHeight; py++) {
            for (size_t px = 0; px < passWidth; px++) {
                size_t outY = STARTING_ROW[pass] + py * ROW_OFFSET[pass];
                size_t outI = (STARTING_COL[pass] + px * COL_OFFSET[pass]) * pixelSize;
                size_t passI = (py*passWidth+px) * pixelSize;
                memcpy(&out[outY][outI], &passImage[passI], pixelSize);
            }
        }
    }

    return Result::OK;
}

PNG::Result PNG::DeinterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height, size_t pixelSize, std::istream& in, std::vector<uint8_t>& out)
{
    out.resize(0);
    switch (method) {
    case InterlaceMethod::NONE:
        return UnfilterPixels(filterMethod, width, height, pixelSize, in, out);
    case InterlaceMethod::ADAM7:
        return Adam7::DeinterlacePixels(filterMethod, width, height, pixelSize, in, out);
    default:
        return Result::UnknownInterlaceMethod;
    }
}
