#include "colour_mapping.hpp"

jpeg::ColourMappedBlockData jpeg::ColourMapper::map(jpeg::BlockGrid::Block const& inputBlock) const{
    return applyMapping(inputBlock);
}

jpeg::BlockGrid::Block jpeg::ColourMapper::unmap(jpeg::ColourMappedBlockData const& inputBlock) const{
    return reverseMapping(inputBlock);
}

bool jpeg::ColourMapper::isLuminanceComponent(uint8_t component) const{
    assert(component < 3);
    return componentIsLuminance(component);
}

jpeg::ColourMappedBlockData jpeg::RGBToRGBMapper::applyMapping(jpeg::BlockGrid::Block const& inputBlock) const{
    ColourMappedBlockData output;
    // Passthrough mapping
    for (size_t i = 0 ; i < inputBlock.m_blockPixelData.size() ; ++i){
        output.m_data[0][i] = inputBlock.m_blockPixelData[i].r;
        output.m_data[1][i] = inputBlock.m_blockPixelData[i].g;
        output.m_data[2][i] = inputBlock.m_blockPixelData[i].b;
    }
    return output;
}

jpeg::BlockGrid::Block jpeg::RGBToRGBMapper::reverseMapping(ColourMappedBlockData const& inputBlock) const{
    BlockGrid::Block output;
    // Passthrough mapping
    for (size_t i = 0 ; i < inputBlock.m_data[0].size() ; ++i){
        output.m_blockPixelData[i].r = inputBlock.m_data[0][i];
        output.m_blockPixelData[i].g = inputBlock.m_data[1][i];
        output.m_blockPixelData[i].b = inputBlock.m_data[2][i];
    }
    return output;
}

/* R, G and B are each luminance components */
bool jpeg::RGBToRGBMapper::componentIsLuminance(uint8_t component) const{
    (void)component;
    return true;
}

jpeg::ColourMappedBlockData jpeg::RGBToYCbCrMapper::applyMapping(jpeg::BlockGrid::Block const& inputBlock) const{
    ColourMappedBlockData output;
    // Conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
    // Range mapping from ITU-T T.871
    auto round = [](double in){return std::floor(in + 0.5);};
    auto mapToRange = [round](double in){
        return uint8_t(std::min(std::max(0.0, round(in)), 255.0));
    };
    auto Y = [mapToRange](BitmapImageRGB::PixelData rgb){
        return mapToRange(0 + 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
    };
    auto Cb = [mapToRange](BitmapImageRGB::PixelData rgb){
        return mapToRange(128 - 0.168736 * rgb.r - 0.331264 * rgb.g + 0.5 * rgb.b);
    };
    auto Cr = [mapToRange](BitmapImageRGB::PixelData rgb){
        return mapToRange(128 + 0.5 * rgb.r - 0.418688 * rgb.g - 0.081312 * rgb.b);
    };
    for (size_t i = 0 ; i < inputBlock.m_blockPixelData.size() ; ++i){
        output.m_data[0][i] = Y(inputBlock.m_blockPixelData[i]);
        output.m_data[1][i] = Cb(inputBlock.m_blockPixelData[i]);
        output.m_data[2][i] = Cr(inputBlock.m_blockPixelData[i]);
    }
    return output;
}

jpeg::BlockGrid::Block jpeg::RGBToYCbCrMapper::reverseMapping(ColourMappedBlockData const& inputBlock) const{
    BlockGrid::Block output;
    // Reverse conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
    // Range mapping from ITU-T T.871
    auto round = [](double in){return std::floor(in + 0.5);};
    auto mapToRange = [round](double in){
        return uint8_t(std::min(std::max(0.0, round(in)), 255.0));
    };
    auto R = [mapToRange](uint8_t Y/* , uint8_t Cb */, uint8_t Cr){
        return mapToRange(Y + 1.402 * (Cr - 128));
    };
    auto G = [mapToRange](uint8_t Y, uint8_t Cb, uint8_t Cr){
        return mapToRange(Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128));
    };
    auto B = [mapToRange](uint8_t Y, uint8_t Cb/* , uint8_t Cr */){
        return mapToRange(Y + 1.772 * (Cb - 128));
    };
    for (size_t i = 0 ; i < inputBlock.m_data[0].size() ; ++i){
        output.m_blockPixelData[i].r = R(inputBlock.m_data[0][i]/* ,inputBlock.data[1][i] */,inputBlock.m_data[2][i]);
        output.m_blockPixelData[i].g = G(inputBlock.m_data[0][i],inputBlock.m_data[1][i],inputBlock.m_data[2][i]);
        output.m_blockPixelData[i].b = B(inputBlock.m_data[0][i],inputBlock.m_data[1][i]/* ,inputBlock.data[2][i] */);
    }
    return output;
}

/* Y is a luminance component, Cb and Cr are chrominance components */
bool jpeg::RGBToYCbCrMapper::componentIsLuminance(uint8_t component) const{
    return component == 0;
}