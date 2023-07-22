#include "png/buffer.h"

PNG::Result PNG::ReadBuffer(std::istream& input, void* buf, size_t bufLen)
{
    if (!input.read((char*)buf, bufLen)) {
        if (input.eof())
            return Result::UnexpectedEOF;
        return Result::Unknown;
    }
    return Result::OK;
}


PNG::Result PNG::ReadUntilEOF(std::istream& input, std::vector<uint8_t>& out)
{
    char ch;
    while (input.read(&ch, 1))
        out.push_back(ch);
    if (input.eof())
        return Result::OK;
    return Result::Unknown;
}
