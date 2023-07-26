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
    case Result::UnknownNecessaryChunk:
        return "UnknownNecessaryChunk";
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
    case Result::CorruptedChunk:
        return "CorruptedChunk";
    case Result::DuplicatePalette:
        return "DuplicatePalette";
    case Result::IllegalPaletteChunk:
        return "IllegalPaletteChunk";
    case Result::InvalidPaletteSize:
        return "InvalidPaletteSize";
    case Result::PaletteNotFound:
        return "PaletteNotFound";
    case Result::InvalidPaletteIndex:
        return "InvalidPaletteIndex";
    case Result::UpdatingClosedStreamError:
        return "UpdatingClosedStreamError";
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
        PNG_UNREACHABLEF("PNG::ResultToString case missing (%d).", (int)res);
    }
}
