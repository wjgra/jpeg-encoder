#ifndef _JPEG_JPEG_IMAGE_HPP_
#define _JPEG_JPEG_IMAGE_HPP_

#include <cstdint>
#include <vector>
#include <array>

#include "..\inc\bitstream.hpp"

// temp
#include "..\inc\entropy_encoder.hpp"
namespace jpeg{
    /* Stores a JPEG image */
    struct JPEGImage{
        ~JPEGImage(){std::cout << "JPEGImage dtor\n";};
        uint16_t width, height;
        struct BlockData{
            struct ScanData{
                // Temp data to allow short-circuit testing
                /* ColourMappedBlock tempColMapBlock;
                DCTChannelOutput tempDCT;
                QuantisedChannelOutput tempQuantised; */
                RunLengthEncodedChannelOutput tempRLE; // one block-channel
            };
            std::array<ScanData, 3> components;
        };
        std::vector<BlockData> data;
    };
};

#endif