#include "png/compression.h"

#ifdef PNG_USE_ZLIB

#include <zlib/zlib.h>

int PNG::ZLib::GetLevel(CompressionLevel l)
{
    switch (l)
    {
    case CompressionLevel::NoCompression:
        return Z_NO_COMPRESSION;
    case CompressionLevel::BestSpeed:
        return Z_BEST_SPEED;
    case CompressionLevel::BestSize:
        return Z_BEST_COMPRESSION;
    default:
        return Z_DEFAULT_COMPRESSION;
    }
}

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
            PNG_LDEBUGF("PNG::ZLib::DecompressData inflated %ldB into %ldB.", inf.total_in, inf.total_out);
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
}

PNG::Result PNG::ZLib::CompressData(IStream& in, OStream& out, CompressionLevel level)
{
    const size_t IN_CAPACITY = 32768; // 32K
    size_t inSize = 0;
    Bytef inBuffer[IN_CAPACITY];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, inBuffer, IN_CAPACITY, &inSize);

    const size_t OUT_CAPACITY = 32768; // 32K
    Bytef outBuffer[OUT_CAPACITY];

    z_stream def;
    def.zalloc = Z_NULL;
    def.zfree = Z_NULL;
    def.opaque = Z_NULL;
    def.avail_in = inSize;
    def.next_in = inBuffer;
    def.avail_out = OUT_CAPACITY;
    def.next_out = outBuffer;

    auto pres = Result::OK;
    deflateInit(&def, GetLevel(level));
    int zcode;
    bool end = false;
    do {
        zcode = deflate(&def, end ? Z_FINISH : Z_NO_FLUSH);
        // Write to the buffer only when a meaningful amount of data is ready.
        if (OUT_CAPACITY - def.avail_out >= OUT_CAPACITY/4) {
            pres = out.WriteBuffer(outBuffer, OUT_CAPACITY-def.avail_out);
            if (pres != Result::OK)
                break;

            // Flush data so that other threads, which have an Input access on `out`, can read it.
            pres = out.Flush();
            if (pres != Result::OK)
                break;

            def.avail_out = OUT_CAPACITY;
            def.next_out = outBuffer;
        }

        if (zcode == Z_STREAM_END) {
            deflateEnd(&def);
            PNG_RETURN_IF_NOT_OK(out.WriteBuffer, outBuffer, OUT_CAPACITY-def.avail_out);
            PNG_RETURN_IF_NOT_OK(out.Flush);
            PNG_LDEBUGF("PNG::ZLib::CompressData deflated %ldB into %ldB.", def.total_in, def.total_out);
            return PNG::Result::OK;
        }

        if (!end && (zcode == Z_BUF_ERROR || def.avail_in == 0)) {
            if (def.avail_in != 0)
                memmove(inBuffer, def.next_in, def.avail_in);

            pres = in.ReadBuffer(inBuffer+def.avail_in, IN_CAPACITY-def.avail_in, &inSize);
            if (pres == Result::UnexpectedEOF || (pres == Result::OK && inSize == 0)) {
                end = true;
                continue;
            } else if (pres != Result::OK) {
                break;
            }

            inSize += def.avail_in;
            def.avail_in = inSize;
            def.next_in = inBuffer;
        }
    } while (zcode == Z_OK || zcode == Z_BUF_ERROR);
    deflateEnd(&def);

    if (pres != Result::OK)
        return pres;

    PNG_LDEBUGF("PNG::ZLib::CompressData error code (%d).", zcode);
    if (def.msg)
        PNG_LDEBUGF("PNG::ZLib::CompressData error message: %s.", def.msg);

    return Result::ZLib_DataError;
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

PNG::Result PNG::CompressData(uint8_t method, IStream& in, OStream& out, CompressionLevel level)
{
    switch (method) {
    case CompressionMethod::ZLIB:
#ifdef PNG_USE_ZLIB
        return ZLib::CompressData(in, out, level);
#else // PNG_USE_ZLIB
        (void) in;
        (void) out;
        (void) level;
        return Result::ZLib_NotAvailable;
#endif // PNG_USE_ZLIB
    default:
        return Result::UnknownCompressionMethod;
    }
}
