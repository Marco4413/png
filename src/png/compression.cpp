#include "png/compression.h"

#ifdef PNG_USE_ZLIB

#include <zlib/zlib.h>

PNG::Result PNG::ZLib::DecompressData(const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    const size_t K32 = 32768; // 32K of data
    out.resize(K32);

    z_stream inf;
    inf.zalloc = Z_NULL;
    inf.zfree = Z_NULL;
    inf.opaque = Z_NULL;
    inf.avail_in = in.size();
    inf.next_in = (uint8_t*)in.data();
    inf.avail_out = out.size();
    inf.next_out = out.data();

    inflateInit(&inf);
    int zcode;
    do {
        zcode = inflate(&inf, Z_NO_FLUSH);
        if (inf.avail_out == 0) {
            out.resize(out.size() + K32);
            inf.avail_out = K32;
            inf.next_out = out.data() + out.size() - K32;
        }
    } while (zcode == Z_OK);
    inflateEnd(&inf);

    if (zcode == Z_STREAM_END) {
        out.resize(out.size() - inf.avail_out);
        return Result::OK;
    }

    return Result::ZLib_DataError;

    /* TODO: Custom ZLib implementation
    // ZLib RFC-1950 http://www.zlib.org/rfc1950.pdf
    uint8_t CMF, FLG;
    PNG_RETURN_IF_NOT_OK(ReadU8, in, CMF);
    PNG_RETURN_IF_NOT_OK(ReadU8, in, FLG);

    // The first 4 bits of CMF indicate the Compression Method (CM) used
    // PNG only supports CM = 8
    if ((CMF & 0x0f) != 8)
        return Result::ZLib_InvalidCompressionMethod;
    
    // The last 4 bits of CMF indicate Compression Info (CINFO) used
    // CINFO is log2(windowSize) - 8. PNG supports a window size up to 32K
    size_t windowSize = 1 << ((CMF >> 4) + 8);
    if (windowSize > 32768)
        return Result::ZLib_InvalidLZ77WindowSize;
    
    { // FCHECK
        uint16_t CMF_FLG = (((uint16_t)CMF) << 8) + FLG;
        if (CMF_FLG % 31 != 0)
            return Result::ZLib_FCHECKFailed;
    }

    bool FDICT = FLG & 0x20;
    PNG_ASSERT(!FDICT, "PNG::DecompressZLibData FDICT not supported.");
    std::cout << (windowSize) << std::endl;

    // DEFLATE RFC-1951 https://www.rfc-editor.org/rfc/pdfrfc/rfc1951.txt.pdf
    */
}

#endif // PNG_USE_ZLIB

PNG::Result PNG::DecompressData(uint8_t method, const std::vector<uint8_t>& in, std::vector<uint8_t>& out)
{
    switch (method) {
    case CompressionMethod::ZLIB:
#ifdef PNG_USE_ZLIB
        return ZLib::DecompressData(in, out);
#else // PNG_USE_ZLIB
        (void) in;
        (void) out;
        return Result::ZLib_NotAvailable;
#endif // PNG_USE_ZLIB
    default:
        return Result::UnknownCompressionMethod;
    }
}
