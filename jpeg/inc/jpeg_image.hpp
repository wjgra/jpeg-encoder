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
        uint16_t width, height;
        std::uintmax_t fileSize;
/*         struct BlockData{
            struct ScanData{
                RunLengthEncodedChannelOutput tempRLE; // one block-channel
            };
            std::array<ScanData, 3> components;
        };
        std::vector<BlockData> data; */


        BitStream compressedImageData;
    };
};

#endif