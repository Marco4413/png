#include "png/base.h"

const char* PNG::ResultToString(Result res)
{
    switch (res) {
    case Result::OK:
        return "OK";
    case Result::EndOfFile:
        return "EndOfFile";
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
    case Result::UnsupportedColorType:
        return "UnsupportedColorType";
    case Result::CorruptedChunk:
        return "CorruptedChunk";
    case Result::IllegalIHDRChunk:
        return "IllegalIHDRChunk";
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
    case Result::IllegalIDATChunk:
        return "IllegalIDATChunk";
    case Result::IllegalIDATSize:
        return "IllegalIDATSize";
    case Result::IllegaltRNSChunk:
        return "IllegaltRNSChunk";
    case Result::InvalidtRNSSize:
        return "InvalidtRNSSize";
    case Result::InvalidTextualDataKeywordSize:
        return "InvalidTextualDataKeywordSize";
    case Result::IllegaltIMEChunk:
        return "IllegaltIMEChunk";
    case Result::InvalidLMTMonth:
        return "InvalidLMTMonth";
    case Result::InvalidLMTDay:
        return "InvalidLMTDay";
    case Result::InvalidLMTHour:
        return "InvalidLMTHour";
    case Result::InvalidLMTMinute:
        return "InvalidLMTMinute";
    case Result::InvalidLMTSecond:
        return "InvalidLMTSecond";
    case Result::UpdatingClosedStreamError:
        return "UpdatingClosedStreamError";
    case Result::ZLib_NotAvailable:
        return "ZLib_NotAvailable";
    case Result::ZLib_DataError:
        return "ZLib_DataError";
    default:
        PNG_UNREACHABLEF("PNG::ResultToString case missing ({}).", (int)res);
    }
}
