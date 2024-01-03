#ifndef _JPEG_JPEG_IMAGE_HPP_
#define _JPEG_JPEG_IMAGE_HPP_

#include <cstdint>
// #include <vector>

namespace jpeg{
    /* Stores a JPEG image */

    struct JPEGSegment{
        // Marker first byte is always 0xFF
        uint8_t markerSecondByte;
        uint16_t length;
        // values
    };

    struct JPEGImage{
        uint16_t width, height;
        
    };
};

#endif