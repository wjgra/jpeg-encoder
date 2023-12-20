#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"

namespace jpeg{
    class Encoder{
    public:
        Encoder(BitmapImage const& inputImage);
        void saveJPEGToFile(std::string const& savePath);
        JPEGImage getJPEGImageData();
    };
};

#endif