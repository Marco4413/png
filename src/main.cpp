#include <assert.h>
#include <fstream>

#include "png/png.h"

// #define MANUAL_READING

#ifdef MANUAL_READING

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
        if (!pngok) { \
            printf("%s:%d: Assert (pngok) failed: %s -> %s\n", __FILE__, __LINE__, #func, PNG::ResultToString(pngcode)); \
            exit(1); \
        } \
    } while (0)

class ScopeTimer
{
public:
    ScopeTimer(const char* name)
        : m_Name(name), m_Start(std::chrono::high_resolution_clock::now()) { }

    ~ScopeTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - m_Start);
        printf("%s took %ldus\n", m_Name, dur.count());
    }

private:
    const char* m_Name;
    const std::chrono::high_resolution_clock::time_point m_Start;
};

int main(int argc, char** argv)
{
    (void)argc;

    assert(*argv++ != NULL);
    const char* filePath = *argv++;
    if (filePath == NULL) {
        printf("Please specify a file to open.\n");
        return 1;
    }

    std::ifstream file(filePath, std::ios::binary);
    PNG::IStreamWrapper inFile(file);

#ifndef MANUAL_READING
    {
        PNG::Image img;
        ScopeTimer t("Single Threaded Image Reading");
        ASSERT_OK(PNG::Image::Read, inFile, img);
    }

    file.seekg(0);
    PNG::Image img;
    {
        ScopeTimer t("Multi Threaded Image Reading");
        // NOTE: Debug Logging can change timings 
        ASSERT_OK(PNG::Image::ReadMT, inFile, img, std::chrono::milliseconds(2));
    }

    /*
    Some benchmarks I have done (while using a non-debug build):
    2560x1440 (449 IDAT, NI, t1) ->
        ST 588ms
        MT 400ms
    2560x1440 (511 IDAT, A7, t2) ->
        ST 570ms
        MT 390ms
    800x600 (1 IDAT, NI, t1) ->
        ST 53ms
        MT 49ms
    800x600 (30 IDAT, NI, t1) ->
        ST 72ms
        MT 61ms
    800x600 (39 IDAT, A7, t1) ->
        ST 78ms
        MT 65ms
    
    NI: No Interlace
    A7: Adam7 Interlace
    tx: Timeout of x ms (for multi-threaded timing)

    The timeout is not perfect, sometimes data does not get produced fast enough for other threads to read.
    However, this will depend on the CPU it is running on. So far the only tests that caused some issues were the ones that use A7.
    */

    size_t x = img.GetWidth() / 2;
    size_t y = img.GetHeight() / 2;
    const auto& pixel = img[y][x];
    
    printf("Pixel at %ld, %ld\n", x, y);
    printf("  R: %ld\n", (size_t)(pixel.R*255));
    printf("  G: %ld\n", (size_t)(pixel.G*255));
    printf("  B: %ld\n", (size_t)(pixel.B*255));
    printf("  A: %ld\n", (size_t)(pixel.A*255));

#else // MANUAL_READING
    uint8_t sig[PNG::PNG_SIGNATURE_LEN];
    ASSERT_OK(inFile.ReadBuffer, sig, PNG::PNG_SIGNATURE_LEN);
    bool isPNGFile = memcmp(sig, PNG::PNG_SIGNATURE, PNG::PNG_SIGNATURE_LEN) == 0;
    assert(isPNGFile && "Input file is not a PNG file.");

    printf("Input file has PNG signature.\n");

    // Reading PNG Header (IHDR)
    PNG::Chunk chunk;
    PNG::IHDRChunk ihdr;
    ASSERT_OK(PNG::Chunk::Read, inFile, chunk);
    ASSERT_OK(PNG::IHDRChunk::Parse, chunk, ihdr);
    printf("Image is %dx%d with a bit depth of %d\n", ihdr.Width, ihdr.Height, ihdr.BitDepth);

    // Vector holding all IDATs (Deflated Image Data)
    PNG::DynamicByteStream defIDAT;
    while (chunk.Type != PNG::ChunkType::IEND) {
        ASSERT_OK(PNG::Chunk::Read, inFile, chunk);
        bool isAux = PNG::ChunkType::IsAncillary(chunk.Type);

        uint32_t readableChunkType = REVERSE_U32(chunk.Type);
        printf("%.4s -> 0x%x (%s)\n", (char*)&readableChunkType, chunk.Type, isAux ? "Aux" : "Not Aux");

        switch (chunk.Type) {
        case PNG::ChunkType::IDAT:
            ASSERT_OK(defIDAT.WriteBuffer, chunk.Data, chunk.Length);
            ASSERT_OK(defIDAT.Flush);
        case PNG::ChunkType::IEND:
            break;
        default:
            // PLTE Chunk from the libpng standard isn't yet implemented.
            assert(isAux);
        }
    }
    printf("Compressed IDAT: %ld\n", defIDAT.GetBuffer().size());

    // Inflating IDAT
    PNG::DynamicByteStream infIDAT;
    ASSERT_OK(PNG::DecompressData, ihdr.CompressionMethod, defIDAT, infIDAT);
    printf("Uncompressed IDAT: %ld\n", infIDAT.GetBuffer().size());

    // Removing Filter from Inflated IDAT
    // size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    // std::vector<uint8_t> pixels;
    // ASSERT_OK(PNG::UnfilterPixels, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, infIDAT, pixels);
    // printf("Unfitlered IDAT: %ld\n", pixels.size());
    
    size_t pixelSize = PNG::ColorType::GetBytesPerPixel(ihdr.ColorType, ihdr.BitDepth);
    std::vector<uint8_t> pixels;
    ASSERT_OK(PNG::DeinterlacePixels, ihdr.InterlaceMethod, ihdr.FilterMethod, ihdr.Width, ihdr.Height, pixelSize, infIDAT, pixels);
    printf("Deinterlaced IDAT: %ld\n", pixels.size());

    size_t x = ihdr.Width / 2; // 397
    size_t y = ihdr.Height / 2; // 302
    uint8_t* pixel = &pixels[y * ihdr.Width * pixelSize + x * pixelSize];

    PNG_ASSERT(ihdr.ColorType == PNG::ColorType::RGBA, "Image is not RGBA.");
    PNG_ASSERT(ihdr.BitDepth > 8, "Image has a Bit Depth > 8.");
    printf("Pixel at %ld, %ld\n", x, y);
    printf("  R: %ld\n", (size_t)pixel[0]);
    printf("  G: %ld\n", (size_t)pixel[1]);
    printf("  B: %ld\n", (size_t)pixel[2]);
    printf("  A: %ld\n", (size_t)pixel[3]);
#endif // MANUAL_READING

    return 0;
}
