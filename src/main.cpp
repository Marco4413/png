#include <assert.h>
#include <fstream>
#include <functional>

#include "png/png.h"

#define ASSERT_OK(func, ...) \
    do { \
        auto pngcode = (func)(__VA_ARGS__); \
        bool pngok = pngcode == PNG::Result::OK; \
        if (!pngok) { \
            PNG_PRINTF(fmt::fg(fmt::color::red), "{}:{}: Assert (pngok) failed: {} -> {}\n", __FILE__, __LINE__, #func, PNG::ResultToString(pngcode)); \
            std::exit(1); \
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
        PNG_PRINTF(fmt::fg(fmt::color::yellow), "{} took {}us.\n", name, totalTime);
    else
        PNG_PRINTF(fmt::fg(fmt::color::yellow), "{} took an average of {}us on {} samples.\n", name, totalTime / samples, samples);
}

int main(int argc, char** argv)
{
    (void)argc;

    assert(*argv++ != NULL);
    const char* filePath = *argv++;
    if (filePath == NULL) {
        PNG_PRINTF("Please specify a file to open.\n");
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

    // A list of keywords can be found at http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html#C.Anc-text
    PNG::Metadata_T metadata;
    metadata.emplace_back(PNG::TextualData {
        .Keyword = "Software",
        .Text = "https://github.com/Marco4413/png",
    });

    const PNG::ExportSettings exportSettings {
        .Metadata = &metadata,
        .ColorType = PNG::ColorType::RGBA,
        .BitDepth = 8,
        .Palette = nullptr,
        .DitheringMethod = PNG::DitheringMethod::None,
        .CompressionLevel = PNG::CompressionLevel::Default,
        .InterlaceMethod = PNG::InterlaceMethod::NONE,
        .IDATSize = 32768,
    };

    bench("Single Threaded Image Writing", [&img, &exportSettings]() {
        std::ofstream file("out.png", std::ios::binary);
        PNG::OStreamWrapper outStream(file);
        ASSERT_OK(img.Write, outStream, exportSettings);
    });

    bench("Multi Threaded Image Writing", [&img, &exportSettings]() {
        std::ofstream file("out-mt.png", std::ios::binary);
        PNG::OStreamWrapper outStream(file);
        ASSERT_OK(img.WriteMT, outStream, exportSettings);
    });

    size_t x = img.GetWidth() / 2;
    size_t y = img.GetHeight() / 2;
    const auto& pixel = img[y][x];

    PNG_PRINTF("Pixel at {}, {}\n", x, y);
    PNG_PRINTF("  R: {}\n", (size_t)(pixel.R*255));
    PNG_PRINTF("  G: {}\n", (size_t)(pixel.G*255));
    PNG_PRINTF("  B: {}\n", (size_t)(pixel.B*255));
    PNG_PRINTF("  A: {}\n", (size_t)(pixel.A*255));

    return 0;
}
