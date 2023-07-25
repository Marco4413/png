#include "png/chunk.h"

#include "png/crc.h"

uint32_t PNG::Chunk::CalculateCRC() const
{
    uint32_t c = CRC::Update(~0, Type);
    return ~CRC::Update(c, Data, Length);
}

PNG::Result PNG::Chunk::Read(IStream& in, PNG::Chunk& chunk)
{
    if (chunk.Data) {
        delete[] chunk.Data;
        chunk.Data = nullptr;
    }
    chunk.Length = 0;

    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.Length);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.Type);
    chunk.Data = new uint8_t[chunk.Length];
    PNG_RETURN_IF_NOT_OK(in.ReadBuffer, chunk.Data, chunk.Length);
    PNG_RETURN_IF_NOT_OK(in.ReadU32, chunk.CRC);

    return Result::OK;
}

PNG::Result PNG::IHDRChunk::Parse(const PNG::Chunk& chunk, PNG::IHDRChunk& ihdr)
{
    if (chunk.Type != ChunkType::IHDR)
        return Result::UnexpectedChunkType;
    
    ByteStream inIHDR(chunk.Data, chunk.Length);

    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU32, ihdr.Width);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU32, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.BitDepth);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.ColorType);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.CompressionMethod);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.FilterMethod);
    PNG_RETURN_IF_NOT_OK(inIHDR.ReadU8, ihdr.InterlaceMethod);

    return Result::OK;
}
