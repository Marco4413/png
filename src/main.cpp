#include <assert.h>
#include <fstream>
#include <functional>

#include "png/png.h"

#define ASSERT_OK(func, ...) \
    do { \
        auto pngcode = (func)(__VA_ARGS__); \
        bool pngok = pngcode == PNG::Result::OK; \
        if (!pngok) { \
            printf("%s:%d: Assert (pngok) failed: %s -> %s\n", __FILE__, __LINE__, #func, PNG::ResultToString(pngcode)); \
            exit(1); \
        } \
    } while (0)

void bench(const char* name, const std::function<void()>& fun, size_t samples = 1)
{
    if (samples == 0)
        return;
    
    size_t totalTime = 0;
    for (size_t i = 0; i < samples; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        fun();
        auto end = std::chrono::high_resolution_clock::now();
        totalTime += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    if (samples == 1)
        printf("%s took %ldus.\n", name, totalTime);
    else
        printf("%s took an average of %ldus on %ld samples.\n", name, totalTime / samples, samples);
}

int main(int argc, char** argv)
{
    (void)argc;

    assert(*argv++ != NULL);
    const char* filePath = *argv++;
    if (filePath == NULL) {
        printf("Please specify a file to open.\n");
        return 1;
    }

    bench("Single Threaded Image Reading", [&filePath]() {
        std::ifstream file(filePath, std::ios::binary);
        PNG::IStreamWrapper inFile(file);
        PNG::Image img;
        ASSERT_OK(PNG::Image::Read, inFile, img);
    });

    PNG::Image img;
    bench("Multi Threaded Image Reading", [&filePath, &img]() {
        std::ifstream file(filePath, std::ios::binary);
        PNG::IStreamWrapper inFile(file);
        ASSERT_OK(PNG::Image::ReadMT, inFile, img);
    });

    // img.Resize(img.GetWidth() * 2, img.GetHeight() * 2, PNG::ScalingMethod::Bilinear);

    bench("Single Threaded Image Writing", [&img]() {
        std::ofstream file("out/out.png", std::ios::binary);
        PNG::OStreamWrapper outStream(file);
        ASSERT_OK(img.Write, outStream, PNG::ColorType::RGBA, 8, nullptr, PNG::DitheringMethod::None,
            PNG::CompressionLevel::Default, PNG::InterlaceMethod::NONE);
    });

    bench("Multi Threaded Image Writing", [&img]() {
        std::ofstream file("out/out-mt.png", std::ios::binary);
        PNG::OStreamWrapper outStream(file);
        ASSERT_OK(img.WriteMT, outStream, PNG::ColorType::RGBA, 8, nullptr, PNG::DitheringMethod::None,
            PNG::CompressionLevel::Default, PNG::InterlaceMethod::NONE);
    });

    /*
    Commit @4f560aa
    These benchmarks are done using the O3 optimized build, and their timings are the average of 10 runs.
    Writing has been done using ColorType::RGBA, 8bit depth, CompressionLevel::Default and InterlaceMethod::None.
    Only 2 images were used, and Writing benchmarks are only stated once per image.
    2560x1440 (449 IDAT, NI) ->
        R-ST 290ms
        R-MT 212ms
        W-ST 1.479s
        W-MT 914ms
    2560x1440 (511 IDAT, A7) ->
        R-ST 341ms
        R-MT 232ms
    800x600 (1 IDAT, NI) ->
        R-ST 28ms
        R-MT 18ms
        W-ST 125ms
        W-MT 68ms
    800x600 (30 IDAT, NI) ->
        R-ST 32ms
        R-MT 24ms
    800x600 (39 IDAT, A7) ->
        R-ST 37ms
        R-MT 27ms
    
    R: Reading
    W: Writing
    ST: Single Threaded
    MT: Multi Threaded
    NI: No Interlace
    A7: Adam7 Interlace
    */

    size_t x = img.GetWidth() / 2;
    size_t y = img.GetHeight() / 2;
    const auto& pixel = img[y][x];
    
    printf("Pixel at %ld, %ld\n", x, y);
    printf("  R: %ld\n", (size_t)(pixel.R*255));
    printf("  G: %ld\n", (size_t)(pixel.G*255));
    printf("  B: %ld\n", (size_t)(pixel.B*255));
    printf("  A: %ld\n", (size_t)(pixel.A*255));

    return 0;
}
