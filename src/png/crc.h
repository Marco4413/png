#pragma once

#ifndef _PNG_CRC_H
#define _PNG_CRC_H

#include "png/base.h"

namespace PNG
{
    class CRC
    {
    public:
        static uint32_t Update(uint32_t crc, const void* buf, size_t bufLen);
        static uint32_t Update(uint32_t crc, uint32_t val);
        static uint32_t Calculate(const void* buf, size_t bufLen);
    
    private:
        // Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
        CRC() {
            for (size_t n = 0; n < 256; n++) {
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

        uint32_t m_Table[256]{0};
        static const CRC m_Instance;
    };
}

#endif // _PNG_CRC_H
