#ifndef _JPEG_JPEG_IMAGE_HPP_
#define _JPEG_JPEG_IMAGE_HPP_

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <fstream>

#include "bitstream.hpp"

// temp
#include "entropy_encoder.hpp"
namespace jpeg{
    /* Stores a JPEG image */
    struct JPEGImage{
        uint16_t width, height;
        std::uintmax_t fileSize;
        BitStream compressedImageData;
        bool supportsSaving = false;
        void saveToFile(std::string const& path){
            if (supportsSaving){
                std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::app);
                file.write(reinterpret_cast<char const*>(compressedImageData.getDataPtr()), compressedImageData.getSize());
                file.close();
            }
            else{
                std::cerr << "Saving is not permitted for this configuration. Try using the baseline encoder.\n";
            }
        }
    };
};

#endif