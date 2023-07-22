#pragma once

#ifndef _PNG_BUFFER_H
#define _PNG_BUFFER_H

#include "png/base.h"

namespace PNG
{
    Result ReadBuffer(std::istream& input, void* buf, size_t bufLen);
    Result ReadUntilEOF(std::istream& input, std::vector<uint8_t>& out);

    template<typename T>
    Result ReadNumber(std::istream& input, T& out)
    {
        out = 0;
        uint8_t buf[1];
        for (size_t i = 0; i < sizeof(T); i++) {
            PNG_RETURN_IF_NOT_OK(ReadBuffer, input, buf, 1);
            out = (out << 8) | buf[0];
        }
        return Result::OK;
    }

    inline Result ReadU8(std::istream& input, uint8_t& out) { return ReadBuffer(input, &out, 1); }
    inline Result ReadU32(std::istream& input, uint32_t& out) { return ReadNumber<uint32_t>(input, out); }
    inline Result ReadU64(std::istream& input, uint64_t& out) { return ReadNumber<uint64_t>(input, out); }

    class ByteBuffer : public std::streambuf
    {
    public:
        ByteBuffer(void* begin, void* end)
        {
            setg((char*)begin, (char*)begin, (char*)end);
        }

        ByteBuffer(void* buf, size_t bufLen)
            : ByteBuffer(buf, (char*)buf + bufLen) {}
    };
}

#endif // _PNG_BUFFER_H
