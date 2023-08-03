#pragma once

#ifndef _PNG_CRC_H
#define _PNG_CRC_H

#include "png/base.h"

namespace PNG
{
    class CRC
    {
    public:
        // Code taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
        static uint32_t Update(uint32_t crc, const void* buf, size_t bufLen);
        static uint32_t Update(uint32_t crc, uint32_t val);
        static uint32_t Calculate(const void* buf, size_t bufLen);
    
    private:
        CRC();
        uint32_t m_Table[256]{0};
        static const CRC m_Instance;
    };
}

#endif // _PNG_CRC_H
