#include "..\inc\encoder.hpp"

jpeg::ColourMappedImageData jpeg::RGBToRGBMapper::map(BitmapImage const& bmp) const{
    ColourMappedImageData output{.width = bmp.width, .height = bmp.height};
    output.data.resize(bmp.data.size());
    for (size_t i = 0 ; i < bmp.data.size() ; ++i){
        // Passthrough mapping
        output.data[i] = {bmp.data[i].r, bmp.data[i].g, bmp.data[i].b};
    }
    return output;
}

jpeg::ColourMappedImageData jpeg::RGBToYCbCrMapper::map(BitmapImage const& bmp) const{
    ColourMappedImageData output{.width = bmp.width, .height = bmp.height};
    output.data.resize(bmp.data.size());
    for (size_t i = 0 ; i < bmp.data.size() ; ++i){
        // Conversion from https://en.wikipedia.org/wiki/YCbCr#JPEG_conversion
        auto Y = [](jpeg::BitmapImage::PixelData rgb){
            return uint8_t(0 + 0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
        };
        auto Cb = [](jpeg::BitmapImage::PixelData rgb){
            return uint8_t(128 - 0.168736 * rgb.r - 0.331264 * rgb.g + 0.5 * rgb.b);
        };
        auto Cr = [](jpeg::BitmapImage::PixelData rgb){
            return uint8_t(128 + 0.5 * rgb.r - 0.418688 * rgb.g - 0.081312 * rgb.b);
        };
        output.data[i] = {
            Y(bmp.data[i]),
            Cb(bmp.data[i]),
            Cr(bmp.data[i])
        };
    }
    return output;
}

jpeg::Encoder::Encoder(BitmapImage const& inputImage,
                    ColourMapper const& colourMapper,
                    BlockGenerator const& blockGenerator,
                    DiscreteCosineTransformer const& discreteCosineTransform,
                    Quantiser const& quantisation,
                    EntropyEncoder const& entropyEncoder) :
    colourMapper{colourMapper},
    blockGenerator{blockGenerator},
    discreteCosineTransform{discreteCosineTransform},
    quantisation{quantisation},
    entropyEncoder{entropyEncoder}
{
    auto colourMappedImageData = colourMapper.map(inputImage);
    temp = colourMappedImageData;
    // etc
    // jpegImageData = output of entropy encoder (s.t. mapping?)
}

bool jpeg::Encoder::saveJPEGToFile(std::string const& savePath){
    // translate to binary, save to path
    // return success?
}

jpeg::JPEGImage jpeg::Encoder::getJPEGImageData(){
    return jpegImageData;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImage const& inputImage) : 
    Encoder(inputImage, RGBToYCbCrMapper(), BlockGenerator(), DiscreteCosineTransformer(), Quantiser(), EntropyEncoder())
{
}