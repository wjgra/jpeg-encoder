#include "../inc/colour_mapping.hpp"

jpeg::ColourMappedImageData jpeg::RGBToRGBMapper::map(BitmapImageRGB const& bmp) const{
    ColourMappedImageData output{
        .width = bmp.width, 
        .height = bmp.height, 
        .data = std::vector<std::array<uint8_t, 3>>(bmp.data.size())
    };
    for (size_t i = 0 ; i < bmp.data.size() ; ++i){
        // Passthrough mapping
        output.data[i] = {bmp.data[i].r, bmp.data[i].g, bmp.data[i].b};
    }
    return output;
}

jpeg::ColourMappedImageData jpeg::RGBToYCbCrMapper::map(BitmapImageRGB const& bmp) const{
    ColourMappedImageData output{
        .width = bmp.width, 
        .height = bmp.height, 
        .data = std::vector<std::array<uint8_t, 3>>(bmp.data.size())
    };
    // Conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
    auto Y = [](jpeg::BitmapImageRGB::PixelData rgb){
        return uint8_t(0 + 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
    };
    auto Cb = [](jpeg::BitmapImageRGB::PixelData rgb){
        return uint8_t(128 - 0.168736 * rgb.r - 0.331264 * rgb.g + 0.5 * rgb.b);
    };
    auto Cr = [](jpeg::BitmapImageRGB::PixelData rgb){
        return uint8_t(128 + 0.5 * rgb.r - 0.418688 * rgb.g - 0.081312 * rgb.b);
    };
    for (size_t i = 0 ; i < bmp.data.size() ; ++i){
        output.data[i] = {
            Y(bmp.data[i]),
            Cb(bmp.data[i]),
            Cr(bmp.data[i])
        };
    }
    return output;
}
