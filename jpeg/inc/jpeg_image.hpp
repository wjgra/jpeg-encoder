#ifndef _JPEG_JPEG_IMAGE_HPP_
#define _JPEG_JPEG_IMAGE_HPP_

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <fstream>

#include "bitstream.hpp"

namespace jpeg{
    /* Stores a JPEG image */
    struct JPEGImage{
        uint16_t m_width, m_height;
        std::uintmax_t m_fileSize;
        BitStream m_compressedImageData;
        bool m_supportsSaving = false;
        void saveToFile(std::string const& path){
            if (m_supportsSaving){
                std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::app);
                file.write(reinterpret_cast<char const*>(m_compressedImageData.getDataPtr()), m_compressedImageData.getSize());
                file.close();
            }
            else{
                std::cerr << "Saving is not permitted for this configuration. Try using the baseline encoder.\n";
            }
        }
    };
};

#endif