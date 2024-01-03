#ifndef _JPEG_COLOUR_MAPPING_HPP_
#define _JPEG_COLOUR_MAPPING_HPP_

#include "..\inc\bitmap_image.hpp"
#include <array>
#include <vector>

namespace jpeg{

    struct ColourMappedImageData{
        uint16_t width, height;
        std::vector<std::array<uint8_t, 3>> data;
    };

    class ColourMapper{
    public:
        virtual ColourMappedImageData map(BitmapImageRGB const& inputImage) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    public:
        virtual ColourMappedImageData map(BitmapImageRGB const& bmp) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    public:
       virtual ColourMappedImageData map(BitmapImageRGB const& bmp) const override;
    };

}
#endif