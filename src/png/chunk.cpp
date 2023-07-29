#include "png/chunk.h"

#include "png/crc.h"

uint32_t PNG::Chunk::CalculateCRC() const
{
    uint32_t c = CRC::Update(~0, Type);
    return ~CRC::Update(c, Data.data(), Data.size());
}

PNG::Result PNG::Chunk::Read(IStream& in, PNG::Chunk& chunk)
{
    uint32_t len;
    PNG_RETURN_IF_NOT_OK(in.ReadU32, len);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.Type);
    chunk.Data.resize(len);
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, chunk.Data.data(), len);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.CRC);

    return Result::OK;
}

PNG::Result PNG::Chunk::Write(PNG::OStream& out) const
{
    PNG_RETURN_IF_NOT_OK(out.WriteU32, Length());
    PNG_RETURN_IF_NOT_OK(out.WriteU32, Type);
    PNG_RETURN_IF_NOT_OK(out.WriteVector, Data);
    PNG_RETURN_IF_NOT_OK(out.WriteU32, CRC);
    return Result::OK;
}

PNG::Result PNG::IHDRChunk::Parse(const PNG::Chunk& chunk, PNG::IHDRChunk& ihdr)
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

PNG::Result PNG::IHDRChunk::Write(PNG::Chunk& chunk) const
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
