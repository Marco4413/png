#include "png/compression.h"

#ifdef PNG_USE_ZLIB

#include <zlib/zlib.h>

PNG::Result PNG::ZLib::DecompressData(IStream& in, OStream& out)
{
    const size_t IN_CAPACITY = 32768; // 32K
    size_t inSize = 0;
    Bytef inBuffer[IN_CAPACITY];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, inBuffer, IN_CAPACITY, &inSize);

    const size_t OUT_CAPACITY = 32768; // 32K
    Bytef outBuffer[OUT_CAPACITY];

    /*
    +-------------------------+
    |       IN_CAPACITY       |
    +-------------------------+
    +------------------+
    |      inSize      |
    +------------------+
    +-------+----------+
    | bRead | avail_in |
    +-------+----------+
    ^
    | inBuffer
    */

    z_stream inf;
    inf.zalloc = Z_NULL;
    inf.zfree = Z_NULL;
    inf.opaque = Z_NULL;
    inf.avail_in = inSize;
    inf.next_in = inBuffer;
    inf.avail_out = OUT_CAPACITY;
    inf.next_out = outBuffer;

    auto pres = Result::OK;
    inflateInit(&inf);
    int zcode;
    do {
        zcode = inflate(&inf, Z_NO_FLUSH);
        // Write to the buffer only when a meaningful amount of data is ready.
        if (OUT_CAPACITY - inf.avail_out >= OUT_CAPACITY/4) {
            pres = out.WriteBuffer(outBuffer, OUT_CAPACITY-inf.avail_out);
            if (pres != Result::OK)
                break;

            // Flush data so that other threads, which have an Input access on `out`, can read it.
            pres = out.Flush();
            if (pres != Result::OK)
                break;

            inf.avail_out = OUT_CAPACITY;
            inf.next_out = outBuffer;
        }

        if (zcode == Z_STREAM_END) {
            inflateEnd(&inf);
            PNG_RETURN_IF_NOT_OK(out.WriteBuffer, outBuffer, OUT_CAPACITY-inf.avail_out);
            PNG_RETURN_IF_NOT_OK(out.Flush);
            PNG_LDEBUGF("PNG::ZLib::DecompressData inflated %ld bytes.", inf.total_out);
            return PNG::Result::OK;
        }

        if (zcode == Z_BUF_ERROR || inf.avail_in == 0) {
            if (inf.avail_in != 0)
                memmove(inBuffer, inf.next_in, inf.avail_in);

            pres = in.ReadBuffer(inBuffer+inf.avail_in, IN_CAPACITY-inf.avail_in, &inSize);
            if (pres != Result::OK) {
                break;
            } else if (inSize == 0) {
                pres = Result::UnexpectedEOF;
                break;
            }

            inSize += inf.avail_in;
            inf.avail_in = inSize;
            inf.next_in = inBuffer;
        }
    } while (zcode == Z_OK || zcode == Z_BUF_ERROR);
    inflateEnd(&inf);

    if (pres != Result::OK)
        return pres;

    PNG_LDEBUGF("PNG::ZLib::DecompressData error code (%d).", zcode);
    if (inf.msg)
        PNG_LDEBUGF("PNG::ZLib::DecompressData error message: %s.", inf.msg);

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

PNG::Result PNG::DecompressData(uint8_t method, IStream& in, OStream& out)
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
