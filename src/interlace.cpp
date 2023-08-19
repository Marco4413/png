#include "png/interlace.h"

#include "png/filter.h"

PNG::Result PNG::Adam7::InterlacePixels(uint8_t filterMethod, size_t width, size_t height,
    size_t bitDepth, size_t samples, CompressionLevel clevel, IStream& in, OStream& out)
{
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Decoders.html#D.Progressive-display
    const size_t STARTING_COL[7] { 0, 4, 0, 2, 0, 1, 0 };
    const size_t STARTING_ROW[7] { 0, 0, 4, 0, 2, 0, 1 };
    const size_t COL_OFFSET[7]   { 8, 8, 4, 4, 2, 2, 1 };
    const size_t ROW_OFFSET[7]   { 8, 8, 8, 4, 4, 2, 2 };

    size_t pixelSize = BitsToBytes(bitDepth) * samples;
    std::vector<uint8_t> _img(pixelSize * width * height);
    // To be able to interlace an image we need the full one
    PNG_RETURN_IF_NOT_OK(in.ReadVector, _img);
    ArrayView2D<uint8_t> img(_img.data(), 0, width*pixelSize);

    std::vector<uint8_t> passImage;
    passImage.reserve(_img.size() / 2);
    /*
    _img.size() / 2 should be enough to never have the need to allocate memory
    1 6 4 6 2 6 4 6
    7 7 7 7 7 7 7 7
    5 6 5 6 5 6 5 6
    7 7 7 7 7 7 7 7
    3 6 4 6 3 6 4 6
    7 7 7 7 7 7 7 7
    5 6 5 6 5 6 5 6
    7 7 7 7 7 7 7 7
    Even lines are used by pass 7 so it is half the size
    */

    for (size_t pass = 0; pass < 7; pass++) {
        passImage.resize(0);
        size_t passHeight = 0;
        for (size_t y = STARTING_ROW[pass]; y < height; y += ROW_OFFSET[pass]) {
            passHeight++;
            for (size_t x = STARTING_COL[pass]; x < width; x += COL_OFFSET[pass]) {
                for (size_t i = 0; i < pixelSize; i++)
                    passImage.push_back(img[y][x*pixelSize+i]);
            }
        }

        if (passImage.size() == 0)
            continue;

        // passImage.size() = passWidth * pixelSize * passHeight 
        // passImage.size() / (pixelSize * passHeight) = passWidth
        size_t passWidth = passImage.size() / (pixelSize * passHeight);

        ByteStream passIn(passImage);
        PNG_RETURN_IF_NOT_OK(FilterPixels, filterMethod, passWidth, passHeight, bitDepth*samples, clevel, passIn, out);
    }

    return Result::OK;
}

PNG::Result PNG::Adam7::DeinterlacePixels(uint8_t filterMethod, size_t width, size_t height,
    size_t bitDepth, size_t samples, IStream& in, std::vector<uint8_t>& _out)
{
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Decoders.html#D.Progressive-display
    const size_t STARTING_COL[7] { 0, 4, 0, 2, 0, 1, 0 };
    const size_t STARTING_ROW[7] { 0, 0, 4, 0, 2, 0, 1 };
    const size_t COL_OFFSET[7]   { 8, 8, 4, 4, 2, 2, 1 };
    const size_t ROW_OFFSET[7]   { 8, 8, 8, 4, 4, 2, 2 };

    // PNG::UnfilterPixels also unpacks pixels
    size_t pixelSize = BitsToBytes(bitDepth) * samples;
    _out.resize(width*height*pixelSize);
    ArrayView2D<uint8_t> out(_out.data(), 0, width*pixelSize);

    std::vector<uint8_t> passImage;
    for (size_t pass = 0; pass < 7; pass++) {
        size_t passWidth = width / COL_OFFSET[pass];
        size_t lastRowSize = width % COL_OFFSET[pass];
        if (lastRowSize != 0 && lastRowSize > STARTING_COL[pass])
            passWidth++;

        if (passWidth == 0)
            continue;

        size_t passHeight = height / ROW_OFFSET[pass];
        size_t lastColSize = height % ROW_OFFSET[pass];
        if (lastColSize != 0 && lastColSize > STARTING_ROW[pass])
            passHeight++;

        if (passHeight == 0)
            continue;

        PNG_RETURN_IF_NOT_OK(UnfilterPixels, filterMethod, passWidth, passHeight, bitDepth*samples, in, passImage);

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

PNG::Result PNG::InterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height,
    size_t bitDepth, size_t samples, CompressionLevel clevel, IStream& in, OStream& out)
{
    switch (method) {
    case InterlaceMethod::NONE:
        return FilterPixels(filterMethod, width, height, bitDepth * samples, clevel, in, out);
    case InterlaceMethod::ADAM7:
        return Adam7::InterlacePixels(filterMethod, width, height, bitDepth, samples, clevel, in, out);
    default:
        return Result::UnknownInterlaceMethod;
    }
}

PNG::Result PNG::DeinterlacePixels(uint8_t method, uint8_t filterMethod, size_t width, size_t height,
    size_t bitDepth, size_t samples, IStream& in, std::vector<uint8_t>& out)
{
    out.resize(0);
    switch (method) {
    case InterlaceMethod::NONE:
        return UnfilterPixels(filterMethod, width, height, bitDepth * samples, in, out);
    case InterlaceMethod::ADAM7:
        return Adam7::DeinterlacePixels(filterMethod, width, height, bitDepth, samples, in, out);
    default:
        return Result::UnknownInterlaceMethod;
    }
}
