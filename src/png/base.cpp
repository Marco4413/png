#include "png/base.h"

const char* PNG::ResultToString(Result res)
{
    switch (res) {
    using enum Result;
    case OK:
        return "OK";
    case Unknown:
        return "Unknown";
    case UnexpectedEOF:
        return "UnexpectedEOF";
    case UnexpectedChunkType:
        return "UnexpectedChunkType";
    case UnknownNecessaryChunk:
        return "UnknownNecessaryChunk";
    case UnknownCompressionMethod:
        return "UnknownCompressionMethod";
    case UnknownFilterMethod:
        return "UnknownFilterMethod";
    case UnknownFilterType:
        return "UnknownFilterType";
    case UnknownInterlaceMethod:
        return "UnknownInterlaceMethod";
    case InvalidSignature:
        return "InvalidSignature";
    case InvalidColorType:
        return "InvalidColorType";
    case InvalidBitDepth:
        return "InvalidBitDepth";
    case InvalidPixelBuffer:
        return "InvalidPixelBuffer";
    case InvalidImageSize:
        return "InvalidImageSize";
    case UnsupportedColorType:
        return "UnsupportedColorType";
    case CorruptedChunk:
        return "CorruptedChunk";
    case DuplicatePalette:
        return "DuplicatePalette";
    case IllegalPaletteChunk:
        return "IllegalPaletteChunk";
    case InvalidPaletteSize:
        return "InvalidPaletteSize";
    case PaletteNotFound:
        return "PaletteNotFound";
    case InvalidPaletteIndex:
        return "InvalidPaletteIndex";
    case UpdatingClosedStreamError:
        return "UpdatingClosedStreamError";
    case ZLib_NotAvailable:
        return "ZLib_NotAvailable";
    case ZLib_DataError:
        return "ZLib_DataError";
    default:
        PNG_UNREACHABLEF("PNG::ResultToString case missing (%d).", (int)res);
    }
}
