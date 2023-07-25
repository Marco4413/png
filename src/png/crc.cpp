#include "png/crc.h"

const PNG::CRC PNG::CRC::m_Instance = PNG::CRC();

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
uint32_t PNG::CRC::Update(uint32_t crc, void* _buf, size_t bufLen)
{
    uint8_t* buf = (uint8_t*)_buf;
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

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
uint32_t PNG::CRC::Calculate(void* buf, size_t bufLen)
{
    return ~Update(~0, buf, bufLen);
}
