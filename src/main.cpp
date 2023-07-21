#include <assert.h>
#include <fstream>
#include <iostream>

#include "png/png.h"

// #define MANUAL_READING

#ifdef MANUAL_READING

#include <cstring>

#define REVERSE_U32(u32) \
    ( (u32) << 24) | \
    ( (u32) >> 24) | \
    (((u32) <<  8) & 0x00ff0000) | \
    (((u32) >>  8) & 0x0000ff00)

#endif // MANUAL_READING

#define ASSERT_OK(func, ...) \
    do { \
        auto pngcode = (func)(__VA_ARGS__); \
        bool pngok = pngcode == PNG::Result::OK; \
        PNG_ASSERTF(pngok, "%s -> %s", #func, PNG::ResultToString(pngcode)); \
    } while (0)

int main(int argc, char** argv)
{
    (void)argc;

    assert(*argv++ != NULL);
    const char* filePath = *argv++;
    if (filePath == NULL) {
        std::cout << "Please specify a file to open." << std::endl;
        return 1;
    }

    std::ifstream file(filePath, std::ios::binary);

#ifndef MANUAL_READING
    PNG::Image img;
    ASSERT_OK(PNG::Image::Read, file, img);
    
    size_t x = img.GetWidth() / 2;
    size_t y = img.GetHeight() / 2;
    const auto& pixel = img[y][x];
    
    std::cout << "Pixel at " << x << ", " << y << std::endl;
    std::cout << "  R: " << (size_t)(pixel.R*255) << std::endl;
    std::cout << "  G: " << (size_t)(pixel.G*255) << std::endl;
    std::cout << "  B: " << (size_t)(pixel.B*255) << std::endl;
    std::cout << "  A: " << (size_t)(pixel.A*255) << std::endl;

#else // MANUAL_READING
    uint8_t sig[PNG::PNG_SIGNATURE_LEN];
    ASSERT_OK(PNG::ReadBuffer, file, sig, PNG::PNG_SIGNATURE_LEN);
    bool isPNGFile = memcmp(sig, PNG::PNG_SIGNATURE, PNG::PNG_SIGNATURE_LEN) == 0;
    assert(isPNGFile && "Input file is not a PNG file.");

    std::cout << "Input file has PNG signature." << std::endl;

    // Reading PNG Header (IHDR)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    ASSERT_OK(PNG::Chunk::Read, file, chunk);
    ASSERT_OK(PNG::IHDRChunk::Parse, chunk, ihdr);
    printf("Image is %dx%d with a bit depth of %d\n", ihdr.Width, ihdr.Height, ihdr.BitDepth);

    // Vector holding all IDATs (Deflated Image Data)
    std::vector<uint8_t> defIDAT;
    while (chunk.Type != PNG::ChunkType::IEND) {
        ASSERT_OK(PNG::Chunk::Read, file, chunk);
        bool isAux = PNG::ChunkType::IsAncillary(chunk.Type);

        uint32_t readableChunkType = REVERSE_U32(chunk.Type);
        printf("%.4s -> 0x%x (%s)\n", (char*)&readableChunkType, chunk.Type, isAux ? "Aux" : "Not Aux");

        switch (chunk.Type) {
        case PNG::ChunkType::IDAT: {
            size_t begin = defIDAT.size();
            defIDAT.resize(defIDAT.size() + chunk.Length);
            memcpy(defIDAT.data() + begin, chunk.Data, chunk.Length);
            break;
        }
        case PNG::ChunkType::IEND:
            break;
        default:
            // PLTE Chunk from the libpng standard isn't yet implemented.
            assert(isAux);
        }
    }
    printf("Compressed IDAT: %ld\n", defIDAT.size());

    // Inflating IDAT
    std::vector<uint8_t> infIDAT;
    ASSERT_OK(PNG::DecompressData, ihdr.CompressionMethod, defIDAT, infIDAT);
    printf("Uncompressed IDAT: %ld\n", infIDAT.size());

    // Removing Filter from Inflated IDAT
    // size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    // std::vector<uint8_t> pixels;
    // ASSERT_OK(PNG::UnfilterPixels, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, infIDAT, pixels);
    // printf("Unfitlered IDAT: %ld\n", pixels.size());
    
    PNG::ByteBuffer infIDATBuf(infIDAT.data(), infIDAT.size());
    std::istream inInfIDAT(&infIDATBuf);

    size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    std::vector<uint8_t> pixels;
    ASSERT_OK(PNG::DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, inInfIDAT, pixels);
    printf("Deinterlaced IDAT: %ld\n", pixels.size());

    size_t x = ihdr.Width / 2; // 397
    size_t y = ihdr.Height / 2; // 302
    uint8_t* pixel = &pixels[y * ihdr.Width * pixelSize + x * pixelSize];

    PNG_ASSERT(ihdr.ColorType == PNG::ColorType::RGBA, "Image is not RGBA.");
    std::cout << "Pixel at " << x << ", " << y << std::endl;
    std::cout << "  R: " << (size_t)pixel[0] << std::endl;
    std::cout << "  G: " << (size_t)pixel[1] << std::endl;
    std::cout << "  B: " << (size_t)pixel[2] << std::endl;
    std::cout << "  A: " << (size_t)pixel[3] << std::endl;
#endif // MANUAL_READING

    return 0;
}
