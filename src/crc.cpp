#include "png/crc.h"

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
// I am not smart enough to come up with my own solution.

const PNG::CRC PNG::CRC::m_Instance = PNG::CRC();

PNG::CRC::CRC()
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (size_t k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        m_Table[n] = c;
    }
}

uint32_t PNG::CRC::Update(uint32_t crc, const void* _buf, size_t bufLen)
{
    const uint8_t* buf = (uint8_t*)_buf;
    for (size_t n = 0; n < bufLen; n++)
        crc = m_Instance.m_Table[(crc ^ buf[n]) & 0xff] ^ (crc >> 8);
    return crc;
}

uint32_t PNG::CRC::Update(uint32_t crc, uint32_t value)
{
    uint8_t buf[4];
    for (int i = 0; i < 4; i++)
        buf[i] = value >> ((3-i) * 8);
    return Update(crc, buf, 4);
}

uint32_t PNG::CRC::Calculate(const void* buf, size_t bufLen)
{
    return ~Update(~0, buf, bufLen);
}
