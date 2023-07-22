#include "png/crc.h"

/* TODO: CRC Implementation
// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
PNG::CRC::CRC()
{
    for (size_t i = 0; i < 256; i++) {
        uint32_t val = i;
        for (size_t k = 0; k < 8; k++) {
            if ((val & 1) != 0)
                val = 0xedb88320L ^ (val >> 1);
            else
                val >>= 1;
        }
        m_Table[i] = val;
    }
}

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
uint32_t PNG::CRC::Update(uint32_t crc, void* _buf, size_t bufLen) const
{
    uint8_t* buf = (uint8_t*)_buf;
    for (size_t i = 0; i < bufLen; i++)
        crc = m_Table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    return crc;
}

uint32_t PNG::CRC::Update(uint32_t crc, uint32_t value) const
{
    for (int i = 3; i >= 0; i--) {
        uint8_t byte = (uint8_t)(value >> (i * 8));
        crc = Update(crc, &byte, 1);
    }
    return crc;
}

// Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
inline uint32_t PNG::CRC::Calculate(void* _buf, size_t bufLen) const
{
    return ~Update(~0, _buf, bufLen);
}

uint32_t PNG::Chunk::CalculateCRC(const PNG::CRC& crcDevice) const
{
    uint32_t crc = ~0;
    crc = crcDevice.Update(crc, Length);
    crc = crcDevice.Update(crc, Type);
    crc = crcDevice.Update(crc, Data, Length);
    return ~crc;
}
*/
