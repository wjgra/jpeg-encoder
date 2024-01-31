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
        BitStream compressedImageData;
        bool supportsSaving = false;
        void saveToFile(){
            if (supportsSaving){

            }
            else{
                std::cerr << "Saving is not permitted for this configuration. Try using the baseline encoder.\n";
            }
        }
    };
};

#endif