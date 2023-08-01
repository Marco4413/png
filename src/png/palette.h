#pragma once

#ifndef _PNG_PALETTE_H
#define _PNG_PALETTE_H

#include "png/base.h"
#include "png/color.h"

namespace PNG
{
    namespace Palette
    {
        // Not using std::array because all functions accept std::vector

        // https://en.wikipedia.org/wiki/Web_colors#Web-safe_colors
        const size_t WEB_SAFE_BIT_DEPTH = 8;
        const std::vector<Color> WEB_SAFE {
            Color(0.0, 0.0, 0.0), Color(0.2, 0.0, 0.0), Color(0.4, 0.0, 0.0), Color(0.6, 0.0, 0.0), Color(0.8, 0.0, 0.0), Color(1.0, 0.0, 0.0),
            Color(0.0, 0.0, 0.2), Color(0.2, 0.0, 0.2), Color(0.4, 0.0, 0.2), Color(0.6, 0.0, 0.2), Color(0.8, 0.0, 0.2), Color(1.0, 0.0, 0.2),
            Color(0.0, 0.0, 0.4), Color(0.2, 0.0, 0.4), Color(0.4, 0.0, 0.4), Color(0.6, 0.0, 0.4), Color(0.8, 0.0, 0.4), Color(1.0, 0.0, 0.4),
            Color(0.0, 0.0, 0.6), Color(0.2, 0.0, 0.6), Color(0.4, 0.0, 0.6), Color(0.6, 0.0, 0.6), Color(0.8, 0.0, 0.6), Color(1.0, 0.0, 0.6),
            Color(0.0, 0.0, 0.8), Color(0.2, 0.0, 0.8), Color(0.4, 0.0, 0.8), Color(0.6, 0.0, 0.8), Color(0.8, 0.0, 0.8), Color(1.0, 0.0, 0.8),
            Color(0.0, 0.0, 1.0), Color(0.2, 0.0, 1.0), Color(0.4, 0.0, 1.0), Color(0.6, 0.0, 1.0), Color(0.8, 0.0, 1.0), Color(1.0, 0.0, 1.0),
            Color(0.0, 0.2, 0.0), Color(0.2, 0.2, 0.0), Color(0.4, 0.2, 0.0), Color(0.6, 0.2, 0.0), Color(0.8, 0.2, 0.0), Color(1.0, 0.2, 0.0),
            Color(0.0, 0.2, 0.2), Color(0.2, 0.2, 0.2), Color(0.4, 0.2, 0.2), Color(0.6, 0.2, 0.2), Color(0.8, 0.2, 0.2), Color(1.0, 0.2, 0.2),
            Color(0.0, 0.2, 0.4), Color(0.2, 0.2, 0.4), Color(0.4, 0.2, 0.4), Color(0.6, 0.2, 0.4), Color(0.8, 0.2, 0.4), Color(1.0, 0.2, 0.4),
            Color(0.0, 0.2, 0.6), Color(0.2, 0.2, 0.6), Color(0.4, 0.2, 0.6), Color(0.6, 0.2, 0.6), Color(0.8, 0.2, 0.6), Color(1.0, 0.2, 0.6),
            Color(0.0, 0.2, 0.8), Color(0.2, 0.2, 0.8), Color(0.4, 0.2, 0.8), Color(0.6, 0.2, 0.8), Color(0.8, 0.2, 0.8), Color(1.0, 0.2, 0.8),
            Color(0.0, 0.2, 1.0), Color(0.2, 0.2, 1.0), Color(0.4, 0.2, 1.0), Color(0.6, 0.2, 1.0), Color(0.8, 0.2, 1.0), Color(1.0, 0.2, 1.0),
            Color(0.0, 0.4, 0.0), Color(0.2, 0.4, 0.0), Color(0.4, 0.4, 0.0), Color(0.6, 0.4, 0.0), Color(0.8, 0.4, 0.0), Color(1.0, 0.4, 0.0),
            Color(0.0, 0.4, 0.2), Color(0.2, 0.4, 0.2), Color(0.4, 0.4, 0.2), Color(0.6, 0.4, 0.2), Color(0.8, 0.4, 0.2), Color(1.0, 0.4, 0.2),
            Color(0.0, 0.4, 0.4), Color(0.2, 0.4, 0.4), Color(0.4, 0.4, 0.4), Color(0.6, 0.4, 0.4), Color(0.8, 0.4, 0.4), Color(1.0, 0.4, 0.4),
            Color(0.0, 0.4, 0.6), Color(0.2, 0.4, 0.6), Color(0.4, 0.4, 0.6), Color(0.6, 0.4, 0.6), Color(0.8, 0.4, 0.6), Color(1.0, 0.4, 0.6),
            Color(0.0, 0.4, 0.8), Color(0.2, 0.4, 0.8), Color(0.4, 0.4, 0.8), Color(0.6, 0.4, 0.8), Color(0.8, 0.4, 0.8), Color(1.0, 0.4, 0.8),
            Color(0.0, 0.4, 1.0), Color(0.2, 0.4, 1.0), Color(0.4, 0.4, 1.0), Color(0.6, 0.4, 1.0), Color(0.8, 0.4, 1.0), Color(1.0, 0.4, 1.0),
            Color(0.0, 0.6, 0.0), Color(0.2, 0.6, 0.0), Color(0.4, 0.6, 0.0), Color(0.6, 0.6, 0.0), Color(0.8, 0.6, 0.0), Color(1.0, 0.6, 0.0),
            Color(0.0, 0.6, 0.2), Color(0.2, 0.6, 0.2), Color(0.4, 0.6, 0.2), Color(0.6, 0.6, 0.2), Color(0.8, 0.6, 0.2), Color(1.0, 0.6, 0.2),
            Color(0.0, 0.6, 0.4), Color(0.2, 0.6, 0.4), Color(0.4, 0.6, 0.4), Color(0.6, 0.6, 0.4), Color(0.8, 0.6, 0.4), Color(1.0, 0.6, 0.4),
            Color(0.0, 0.6, 0.6), Color(0.2, 0.6, 0.6), Color(0.4, 0.6, 0.6), Color(0.6, 0.6, 0.6), Color(0.8, 0.6, 0.6), Color(1.0, 0.6, 0.6),
            Color(0.0, 0.6, 0.8), Color(0.2, 0.6, 0.8), Color(0.4, 0.6, 0.8), Color(0.6, 0.6, 0.8), Color(0.8, 0.6, 0.8), Color(1.0, 0.6, 0.8),
            Color(0.0, 0.6, 1.0), Color(0.2, 0.6, 1.0), Color(0.4, 0.6, 1.0), Color(0.6, 0.6, 1.0), Color(0.8, 0.6, 1.0), Color(1.0, 0.6, 1.0),
            Color(0.0, 0.8, 0.0), Color(0.2, 0.8, 0.0), Color(0.4, 0.8, 0.0), Color(0.6, 0.8, 0.0), Color(0.8, 0.8, 0.0), Color(1.0, 0.8, 0.0),
            Color(0.0, 0.8, 0.2), Color(0.2, 0.8, 0.2), Color(0.4, 0.8, 0.2), Color(0.6, 0.8, 0.2), Color(0.8, 0.8, 0.2), Color(1.0, 0.8, 0.2),
            Color(0.0, 0.8, 0.4), Color(0.2, 0.8, 0.4), Color(0.4, 0.8, 0.4), Color(0.6, 0.8, 0.4), Color(0.8, 0.8, 0.4), Color(1.0, 0.8, 0.4),
            Color(0.0, 0.8, 0.6), Color(0.2, 0.8, 0.6), Color(0.4, 0.8, 0.6), Color(0.6, 0.8, 0.6), Color(0.8, 0.8, 0.6), Color(1.0, 0.8, 0.6),
            Color(0.0, 0.8, 0.8), Color(0.2, 0.8, 0.8), Color(0.4, 0.8, 0.8), Color(0.6, 0.8, 0.8), Color(0.8, 0.8, 0.8), Color(1.0, 0.8, 0.8),
            Color(0.0, 0.8, 1.0), Color(0.2, 0.8, 1.0), Color(0.4, 0.8, 1.0), Color(0.6, 0.8, 1.0), Color(0.8, 0.8, 1.0), Color(1.0, 0.8, 1.0),
            Color(0.0, 1.0, 0.0), Color(0.2, 1.0, 0.0), Color(0.4, 1.0, 0.0), Color(0.6, 1.0, 0.0), Color(0.8, 1.0, 0.0), Color(1.0, 1.0, 0.0),
            Color(0.0, 1.0, 0.2), Color(0.2, 1.0, 0.2), Color(0.4, 1.0, 0.2), Color(0.6, 1.0, 0.2), Color(0.8, 1.0, 0.2), Color(1.0, 1.0, 0.2),
            Color(0.0, 1.0, 0.4), Color(0.2, 1.0, 0.4), Color(0.4, 1.0, 0.4), Color(0.6, 1.0, 0.4), Color(0.8, 1.0, 0.4), Color(1.0, 1.0, 0.4),
            Color(0.0, 1.0, 0.6), Color(0.2, 1.0, 0.6), Color(0.4, 1.0, 0.6), Color(0.6, 1.0, 0.6), Color(0.8, 1.0, 0.6), Color(1.0, 1.0, 0.6),
            Color(0.0, 1.0, 0.8), Color(0.2, 1.0, 0.8), Color(0.4, 1.0, 0.8), Color(0.6, 1.0, 0.8), Color(0.8, 1.0, 0.8), Color(1.0, 1.0, 0.8),
            Color(0.0, 1.0, 1.0), Color(0.2, 1.0, 1.0), Color(0.4, 1.0, 1.0), Color(0.6, 1.0, 1.0), Color(0.8, 1.0, 1.0), Color(1.0, 1.0, 1.0),
        };

        const size_t WEB_SAFEST_BIT_DEPTH = 8;
        const std::vector<Color> WEB_SAFEST {
            Color(0.0, 0.0, 0.0), Color(1.0, 0.0, 0.0),
            Color(0.0, 0.0, 0.2), Color(1.0, 0.0, 0.2),
            Color(0.0, 0.0, 1.0), Color(1.0, 0.0, 1.0),
            Color(0.0, 1.0, 0.0), Color(0.4, 1.0, 0.0), Color(1.0, 1.0, 0.0),
            Color(0.2, 1.0, 0.2), Color(0.4, 1.0, 0.2), Color(1.0, 1.0, 0.2),
            Color(0.0, 1.0, 0.4), Color(0.2, 1.0, 0.4), Color(0.8, 1.0, 0.4), Color(1.0, 1.0, 0.4),
            Color(0.0, 1.0, 0.8), Color(0.2, 1.0, 0.8),
            Color(0.0, 1.0, 1.0), Color(0.2, 1.0, 1.0), Color(0.4, 1.0, 1.0), Color(1.0, 1.0, 1.0),
        };
    }
}

#endif // _PNG_PALETTE_H
