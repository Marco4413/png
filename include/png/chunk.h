#pragma once

#ifndef _PNG_CHUNK_H
#define _PNG_CHUNK_H

#include "png/base.h"
#include "png/color.h"
#include "png/compression.h"
#include "png/filter.h"
#include "png/interlace.h"
#include "png/stream.h"

namespace PNG
{
    struct TextualData; // Forward Declaration
    using Metadata_T = std::vector<TextualData>;

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

        const uint32_t tEXt = 0x74455874;
        const uint32_t iTXt = 0x69545874;
        const uint32_t zTXt = 0x7a545874;

        const uint32_t tIME = 0x74494d45;
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

    struct ImageHeader
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint8_t BitDepth = 8;
        uint8_t ColorType = ColorType::RGBA;
        uint8_t CompressionMethod = CompressionMethod::ZLIB;
        uint8_t FilterMethod = FilterMethod::ADAPTIVE_FILTERING;
        uint8_t InterlaceMethod = InterlaceMethod::NONE;

        Result Validate() const;
        static Result Parse(const Chunk& chunk, ImageHeader& ihdr);
        Result Write(Chunk& chunk) const;
    };

    struct TextualData
    {
        std::string Keyword = "";
        std::string LanguageTag = "";
        std::string TranslatedKeyword = "";
        std::string Text = "";
        bool IsUTF8 = false;

        Result Validate() const;
        static Result Parse(const Chunk& chunk, TextualData& textualData);
        Result Write(Chunk& chunk, CompressionLevel compressionLevel = CompressionLevel::Default) const;
    };

    struct LastModificationTime
    {
        uint16_t Year = 0;
        uint8_t Month = 0;
        uint8_t Day = 0;
        uint8_t Hour = 0;
        uint8_t Minute = 0;
        uint8_t Second = 0;

        void Update();
        static LastModificationTime Now();

        Result Validate() const;
        static Result Parse(const Chunk& chunk, LastModificationTime& time);
        Result Write(Chunk& chunk) const;
    };
}

#endif // _PNG_CHUNK_H
