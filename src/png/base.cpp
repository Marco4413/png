#include "png/base.h"

const char* PNG::ResultToString(PNG::Result res)
{
    switch (res) {
    case Result::OK:
        return "OK";
    case Result::Unknown:
        return "Unknown";
    case Result::UnexpectedEOF:
        return "UnexpectedEOF";
    case Result::UnexpectedChunkType:
        return "UnexpectedChunkType";
    case Result::UnknownCompressionMethod:
        return "UnknownCompressionMethod";
    case Result::UnknownFilterMethod:
        return "UnknownFilterMethod";
    case Result::UnknownFilterType:
        return "UnknownFilterType";
    case Result::UnknownInterlaceMethod:
        return "UnknownInterlaceMethod";
    case Result::InvalidSignature:
        return "InvalidSignature";
    case Result::InvalidColorType:
        return "InvalidColorType";
    case Result::InvalidBitDepth:
        return "InvalidBitDepth";
    case Result::InvalidPixelBuffer:
        return "InvalidPixelBuffer";
    case Result::InvalidImageSize:
        return "InvalidImageSize";
    /* TODO: Custom ZLib implementation
    case Result::ZLib_InvalidCompressionMethod:
        return "ZLib_InvalidCompressionMethod";
    case Result::ZLib_InvalidLZ77WindowSize:
        return "ZLib_InvalidLZ77WindowSize";
    */
    case Result::ZLib_NotAvailable:
        return "ZLib_NotAvailable";
    case Result::ZLib_DataError:
        return "ZLib_DataError";
    default:
        PNG_UNREACHABLEF("Missing %d code.", (int)res);
    }
}
