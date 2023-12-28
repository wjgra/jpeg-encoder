#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImage const& inputImage,
                    ColourMapper const& colourMapper,
                    BlockGenerator const& blockGenerator,
                    DiscreteCosineTransformer const& discreteCosineTransform,
                    Quantiser const& quantisation,
                    EntropyEncoder const& entropyEncoder) /* :
    colourMapper{colourMapper},
    blockGenerator{blockGenerator},
    discreteCosineTransform{discreteCosineTransform},
    quantisation{quantisation},
    entropyEncoder{entropyEncoder} */
{
    auto colourMappedImageData = colourMapper.map(inputImage);
    temp = colourMappedImageData;
    // etc
    // jpegImageData = output of entropy encoder (s.t. mapping?)
}

bool jpeg::Encoder::saveJPEGToFile(std::string const& savePath){
    // translate to binary, save to path
    // return success status?
    return false;
}

jpeg::JPEGImage jpeg::Encoder::getJPEGImageData(){
    return jpegImageData;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImage const& inputImage) : 
    Encoder(inputImage, /* RGBToRGBMapper() */RGBToYCbCrMapper(), BlockGenerator(), DiscreteCosineTransformer(), Quantiser(), EntropyEncoder())
{
}