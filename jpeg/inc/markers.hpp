#ifndef _JPEG_MARKERS_HPP_
#define _JPEG_MARKERS_HPP_
#include <cstdint>
namespace jpeg{
    uint16_t const markerStartOfImageSegmentSOI{0xFFD8};
    uint16_t const markerJFIFImageSegmentAPP0{0xFFE0};
    uint16_t const markerCommentSegmentCOM{0xFFFE};
    uint16_t const markerDefineQuantisationTableSegmentDQT{0xFFDB};
    uint16_t const markerStartOfFrame0SOF0{0xFFC0};
    uint16_t const markerDefineHuffmanTableSegmentDHT{0xFFC4};
    uint16_t const markerStartOfScanSegmentSOS{0xFFDA};
    uint16_t const markerEndOfImageSegmentEOI{0xFFD9};
}
#endif