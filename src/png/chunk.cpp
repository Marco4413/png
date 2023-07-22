#include "png/chunk.h"

#include "png/buffer.h"

PNG::Result PNG::Chunk::Read(std::istream& input, PNG::Chunk& chunk)
{
    if (chunk.Data) {
        delete[] chunk.Data;
        chunk.Data = nullptr;
    }
    chunk.Length = 0;

    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.Length);
    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.Type);
    chunk.Data = new uint8_t[chunk.Length];
    PNG_RETURN_IF_NOT_OK(ReadBuffer, input, chunk.Data, chunk.Length);
    PNG_RETURN_IF_NOT_OK(ReadU32, input, chunk.CRC);

    return Result::OK;
}

PNG::Result PNG::IHDRChunk::Parse(const PNG::Chunk& chunk, PNG::IHDRChunk& ihdr)
{
    if (chunk.Type != ChunkType::IHDR)
        return Result::UnexpectedChunkType;
    
    ByteBuffer ihdrBuf(chunk.Data, chunk.Length);
    std::istream inIHDR(&ihdrBuf);

    PNG_RETURN_IF_NOT_OK(ReadU32, inIHDR, ihdr.Width);
    PNG_RETURN_IF_NOT_OK(ReadU32, inIHDR, ihdr.Height);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.BitDepth);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.ColorType);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.CompressionMethod);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.FilterMethod);
    PNG_RETURN_IF_NOT_OK(ReadU8, inIHDR, ihdr.InterlaceMethod);

    return Result::OK;
}
