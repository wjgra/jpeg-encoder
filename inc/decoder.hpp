#ifndef _JPEG_DECODER_HPP_
#define _JPEG_DECODER_HPP_

#include <string>

#include "..\inc\jpeg_image.hpp"
#include "..\inc\bitmap_image.hpp"

namespace jpeg{
    class Decoder{
    public:
        Decoder(JPEGImage inputImage);
        void saveBitmapToFile(std::string const& savePath);
        BitmapImage getBitmapImageData();
    };
};

#endif