#pragma once

#ifndef _PNG_CHUNK_H
#define _PNG_CHUNK_H

#include "png/base.h"
#include "png/stream.h"

namespace PNG
{
    namespace ChunkType
    {
        inline bool IsAncillary(uint32_t chunkType)  { return (chunkType & 0x20000000) != 0; }
        inline bool IsPrivate(uint32_t chunkType)    { return (chunkType & 0x00200000) != 0; }
        inline bool IsConforming(uint32_t chunkType) { return (chunkType & 0x00002000) != 0; }
        inline bool IsSafeToCopy(uint32_t chunkType) { return (chunkType & 0x00000020) != 0; }

        const uint32_t PLTE = 0x504c5445;
        const uint32_t IHDR = 0x49484452;
        const uint32_t IDAT = 0x49444154;
        const uint32_t IEND = 0x49454e44;

        const uint32_t tRNS = 0x74524e53;
    }

    class Chunk
    {
    public:
        uint32_t Type = 0;
        // Data has a max length of 2^32
        std::vector<uint8_t> Data;
        uint32_t CRC = 0;
    public:
        Chunk() = default;

        uint32_t Length() const { return (uint32_t)Data.size(); }
        uint32_t CalculateCRC() const;

        static Result Read(IStream& in, Chunk& chunk);
        Result Write(OStream& out) const;
    };

    class IHDRChunk
    {
    public:
        uint32_t Width;
        uint32_t Height;
        uint8_t BitDepth;
        uint8_t ColorType;
        uint8_t CompressionMethod;
        uint8_t FilterMethod;
        uint8_t InterlaceMethod;

        IHDRChunk() = default;
        IHDRChunk(const IHDRChunk&) = default;
        IHDRChunk& operator=(const IHDRChunk&) = default;
    
        static Result Parse(const Chunk& chunk, IHDRChunk& ihdr);
        Result Write(Chunk& chunk) const;
    };
}

#endif // _PNG_CHUNK_H
