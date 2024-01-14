#include "../inc/colour_mapping.hpp"

jpeg::ColourMappedBlock jpeg::ColourMapper::map(jpeg::BlockGrid::Block const& inputBlock) const{
    return applyMapping(inputBlock);
}

jpeg::BlockGrid::Block jpeg::ColourMapper::unmap(jpeg::ColourMappedBlock const& inputBlock) const{
    return reverseMapping(inputBlock);
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

jpeg::BlockGrid::Block jpeg::RGBToRGBMapper::reverseMapping(ColourMappedBlock const& inputBlock) const{
    BlockGrid::Block output;
    // Passthrough mapping
    for (size_t i = 0 ; i < inputBlock.data[0].size() ; ++i){
        output.data[i].r = inputBlock.data[0][i];
        output.data[i].g = inputBlock.data[1][i];
        output.data[i].b = inputBlock.data[2][i];
    }
    return output;
}

jpeg::ColourMappedBlock jpeg::RGBToYCbCrMapper::applyMapping(jpeg::BlockGrid::Block const& inputBlock) const{
    ColourMappedBlock output;
    // Conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
    auto Y = [](BitmapImageRGB::PixelData rgb){
        return uint8_t(0 + 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
    };
    auto Cb = [](BitmapImageRGB::PixelData rgb){
        return uint8_t(128 - 0.168736 * rgb.r - 0.331264 * rgb.g + 0.5 * rgb.b);
    };
    auto Cr = [](BitmapImageRGB::PixelData rgb){
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

jpeg::BlockGrid::Block jpeg::RGBToYCbCrMapper::reverseMapping(ColourMappedBlock const& inputBlock) const{
    BlockGrid::Block output;
    // Reverse conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
    auto R = [](uint8_t Y/* , uint8_t Cb */, uint8_t Cr){
        return uint8_t(Y + 1.402 * (Cr - 128));
    };
    auto G = [](uint8_t Y, uint8_t Cb, uint8_t Cr){
        return uint8_t(Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128));
    };
    auto B = [](uint8_t Y, uint8_t Cb/* , uint8_t Cr */){
        return uint8_t(Y + 1.772 * (Cb - 128));
    };
    for (size_t i = 0 ; i < inputBlock.data[0].size() ; ++i){
        output.data[i].r = R(inputBlock.data[0][i]/* ,inputBlock.data[1][i] */,inputBlock.data[2][i]);
        output.data[i].g = G(inputBlock.data[0][i],inputBlock.data[1][i],inputBlock.data[2][i]);
        output.data[i].b = B(inputBlock.data[0][i],inputBlock.data[1][i]/* ,inputBlock.data[2][i] */);
    }
    return output;
}