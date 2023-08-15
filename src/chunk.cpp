#include "png/chunk.h"

#include "png/crc.h"
#include "png/compression.h"

uint32_t PNG::Chunk::CalculateCRC() const
{
    uint32_t c = CRC::Update(~0, Type);
    return ~CRC::Update(c, Data.data(), Length());
}

PNG::Result PNG::Chunk::Read(IStream& in, Chunk& chunk)
{
    uint32_t len;
    PNG_RETURN_IF_NOT_OK(in.ReadU32, len);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.Type);
    chunk.Data.resize(len);
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, chunk.Data.data(), len);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.CRC);

    return Result::OK;
}

PNG::Result PNG::Chunk::Write(OStream& out) const
{
    PNG_RETURN_IF_NOT_OK(out.WriteU32, Length());
    PNG_RETURN_IF_NOT_OK(out.WriteU32, Type);
    PNG_RETURN_IF_NOT_OK(out.WriteVector, Data);
    PNG_RETURN_IF_NOT_OK(out.WriteU32, CRC);
    return Result::OK;
}

PNG::Result PNG::ImageHeader::Parse(const Chunk& chunk, ImageHeader& ihdr)
{
    if (chunk.Type != ChunkType::IHDR)
        return Result::UnexpectedChunkType;
    
    ByteStream inIHDR(chunk.Data);

    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU32, ihdr.Width);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU32, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.BitDepth);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.ColorType);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.CompressionMethod);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.FilterMethod);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.InterlaceMethod);

    return Result::OK;
}

PNG::Result PNG::ImageHeader::Write(Chunk& chunk) const
{
    DynamicByteStream stream;
    PNG_RETURN_IF_NOT_OK(stream.WriteU32, Width);
    PNG_RETURN_IF_NOT_OK(stream.WriteU32, Height);
    uint8_t conf[5]{BitDepth, ColorType, CompressionMethod, FilterMethod, InterlaceMethod};
    PNG_RETURN_IF_NOT_OK(stream.WriteBuffer, conf, 5);
    PNG_RETURN_IF_NOT_OK(stream.Flush);
    PNG_RETURN_IF_NOT_OK(stream.Close);

    chunk.Type = ChunkType::IHDR;
    chunk.Data = std::move(stream.GetBuffer());
    chunk.CRC = chunk.CalculateCRC();
    return Result::OK;
}

PNG::Result PNG::TextualData::Validate() const
{
    // Keyword: 1-79 bytes (character string)
    if (Keyword.empty() || Keyword.length() >= 80)
        return Result::InvalidTextualDataKeywordSize;
    return Result::OK;
}

PNG::Result PNG::TextualData::Parse(const Chunk& chunk, TextualData& data)
{
    switch (chunk.Type) {
    case ChunkType::tEXt:
    case ChunkType::zTXt:
    case ChunkType::iTXt:
        break;
    default:
        return Result::UnexpectedChunkType;
    }
    
    ByteStream in(chunk.Data);
    
    // Keyword:        1-79 bytes (character string)
    // Null separator: 1 byte
    PNG_RETURN_IF_NOT_OK(in.ReadString, data.Keyword);
    // NOTE: Validate only checks for the Keyword so it is safe to use here
    PNG_RETURN_IF_NOT_OK(data.Validate);

    if (chunk.Type == ChunkType::tEXt) {
        // Text: n bytes (character string)
        size_t textStart = data.Keyword.length() + 1;
        data.Text.append(chunk.Data.begin()+textStart, chunk.Data.end());
    } else if (chunk.Type == ChunkType::zTXt) {
        // Compression method: 1 byte
        uint8_t compressionMethod;
        PNG_RETURN_IF_NOT_OK(in.ReadU8, compressionMethod);
        // Compressed text:    n bytes
        DynamicByteStream out;
        PNG_RETURN_IF_NOT_OK(DecompressData, compressionMethod, in, out);
        data.Text.append(out.GetBuffer().begin(), out.GetBuffer().end());
    } else {
        data.IsUTF8 = true;
        // Compression flag:   1 byte
        uint8_t compressionFlag;
        PNG_RETURN_IF_NOT_OK(in.ReadU8, compressionFlag);
        // Compression method: 1 byte
        uint8_t compressionMethod;
        PNG_RETURN_IF_NOT_OK(in.ReadU8, compressionMethod);
        // Language tag:       0 or more bytes (character string)
        // Null separator:     1 byte
        PNG_RETURN_IF_NOT_OK(in.ReadString, data.LanguageTag);
        // Translated keyword: 0 or more bytes
        // Null separator:     1 byte
        PNG_RETURN_IF_NOT_OK(in.ReadString, data.TranslatedKeyword);
        // Text:               0 or more bytes
        if (compressionFlag) {
            DynamicByteStream out;
            PNG_RETURN_IF_NOT_OK(DecompressData, compressionMethod, in, out);
            data.Text.append(out.GetBuffer().begin(), out.GetBuffer().end());
        } else {
            data.Text.resize(in.GetAvailable());
            PNG_RETURN_IF_NOT_OK(in.ReadBuffer, data.Text.data(), data.Text.size());
            PNG_ASSERT(in.GetAvailable() == 0, "PNG::TextualData::Parse Could not consume Internal ByteStream.");
        }
    }

    return Result::OK;
}

PNG::Result PNG::TextualData::Write(Chunk& chunk, CompressionLevel compressionLevel) const
{
    constexpr size_t DEFLATE_THRESHOLD = 1024;

    PNG_RETURN_IF_NOT_OK(Validate);
    
    bool deflate = false;
    chunk.Type = ChunkType::tEXt;
    if (IsUTF8 || !LanguageTag.empty() || !TranslatedKeyword.empty())
        chunk.Type = ChunkType::iTXt;

    if (Text.length() >= DEFLATE_THRESHOLD) {
        deflate = true;
        if (chunk.Type == ChunkType::tEXt)
            chunk.Type = ChunkType::zTXt;
    }

    // Keyword:        1-79 bytes (character string)
    // Null separator: 1 byte
    DynamicByteStream out;
    PNG_RETURN_IF_NOT_OK(out.WriteString, Keyword);
    
    switch (chunk.Type) {
    case ChunkType::tEXt:
        // Text: n bytes (character string)
        PNG_RETURN_IF_NOT_OK(out.WriteBuffer, Text.data(), Text.size());
        break;
    case ChunkType::zTXt: {
        // Compression method: 1 byte
        PNG_RETURN_IF_NOT_OK(out.WriteU8, CompressionMethod::ZLIB);
        // Compressed text:    n bytes
        ByteStream textStream(Text.data(), Text.length());
        PNG_RETURN_IF_NOT_OK(CompressData, CompressionMethod::ZLIB, textStream, out, compressionLevel);
        break;
    }
    case ChunkType::iTXt:
        // Compression flag:   1 byte
        PNG_RETURN_IF_NOT_OK(out.WriteU8, deflate ? 1 : 0);
        // Compression method: 1 byte
        PNG_RETURN_IF_NOT_OK(out.WriteU8, CompressionMethod::ZLIB);
        // Language tag:       0 or more bytes (character string)
        // Null separator:     1 byte
        PNG_RETURN_IF_NOT_OK(out.WriteString, LanguageTag);
        // Translated keyword: 0 or more bytes
        // Null separator:     1 byte
        PNG_RETURN_IF_NOT_OK(out.WriteString, TranslatedKeyword);
        // Text:               0 or more bytes
        if (deflate) {
            ByteStream textStream(Text.data(), Text.length());
            PNG_RETURN_IF_NOT_OK(CompressData, CompressionMethod::ZLIB, textStream, out, compressionLevel);
        } else
            PNG_RETURN_IF_NOT_OK(out.WriteBuffer, Text.data(), Text.size());
        break;
    default:
        PNG_UNREACHABLE("PNG::TextualData::Write Chunk.Type was internally set to an invalid value.");
    }

    PNG_RETURN_IF_NOT_OK(out.Flush);
    PNG_RETURN_IF_NOT_OK(out.Close);

    chunk.Data = std::move(out.GetBuffer());
    chunk.CRC = chunk.CalculateCRC();

    return Result::OK;
}
