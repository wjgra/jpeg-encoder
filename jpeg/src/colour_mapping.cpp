#include "../inc/colour_mapping.hpp"

jpeg::ColourMappedBlock jpeg::ColourMapper::map(jpeg::BlockGrid::Block const& inputBlock) const{
    return applyMapping(inputBlock);
}

jpeg::ColourMappedBlock jpeg::RGBToRGBMapper::applyMapping(jpeg::BlockGrid::Block const& inputBlock) const{
    ColourMappedBlock output;
    // Passthrough mapping
    for (size_t i = 0 ; i < inputBlock.data.size() ; ++i){
        // output.data[i] = {inputBlock.data[i].r, inputBlock.data[i].g, inputBlock.data[i].b};
        output.data[0][i] = inputBlock.data[i].r;
        output.data[1][i] = inputBlock.data[i].g;
        output.data[2][i] = inputBlock.data[i].b;
    }
    return output;
}

jpeg::ColourMappedBlock jpeg::RGBToYCbCrMapper::applyMapping(jpeg::BlockGrid::Block const& inputBlock) const{
    ColourMappedBlock output;
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
    for (size_t i = 0 ; i < inputBlock.data.size() ; ++i){
        /* output.data[i] = {
            Y(inputBlock.data[i]),
            Cb(inputBlock.data[i]),
            Cr(inputBlock.data[i])
        }; */
        output.data[0][i] = Y(inputBlock.data[i]);
        output.data[1][i] = Cb(inputBlock.data[i]);
        output.data[2][i] = Cr(inputBlock.data[i]);
    }
    return output;
}