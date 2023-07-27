#include <assert.h>
#include <fstream>

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

    {
        PNG::Image img;
        ScopeTimer t("Single Threaded Image Reading");
        ASSERT_OK(PNG::Image::Read, inFile, img);
    }

    file.seekg(0);
    PNG::Image img;
    {
        ScopeTimer t("Multi Threaded Image Reading");
        ASSERT_OK(PNG::Image::ReadMT, inFile, img);
    }

    {
        std::ofstream oFile("out/out.png", std::ios::binary);
        PNG::OStreamWrapper oStream(oFile);
        ScopeTimer t("Single Threaded Image Writing");
        ASSERT_OK(img.Write, oStream, PNG::CompressionLevel::BestSize);
    }

    /*
    Commit @92ece21
    Some benchmarks I have done (while using the O3 optimized build):
    2560x1440 (449 IDAT, NI) ->
        ST 275ms
        MT 191ms
    2560x1440 (511 IDAT, A7) ->
        ST 302ms
        MT 223ms
    800x600 (1 IDAT, NI) ->
        ST 20ms
        MT 17ms
    800x600 (30 IDAT, NI) ->
        ST 31ms
        MT 23ms
    800x600 (39 IDAT, A7) ->
        ST 34ms
        MT 26ms
    
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
