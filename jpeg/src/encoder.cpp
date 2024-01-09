#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                    ColourMapper const& colourMapper,
                    DiscreteCosineTransformer const& discreteCosineTransform,
                    Quantiser const& quantisation,
                    EntropyEncoder const& entropyEncoder)
{
    BlockGrid blockGrid(inputImage);
    for (auto block : blockGrid){

    }
}

bool jpeg::Encoder::saveJPEGToFile(std::string const& savePath){
    // translate to binary, save to path
    // return success status?
    return false;
}

jpeg::JPEGImage jpeg::Encoder::getJPEGImageData(){
    return jpegImageData;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage) : 
    Encoder(inputImage, /* RGBToRGBMapper() */RGBToYCbCrMapper(), DiscreteCosineTransformer(), Quantiser(), EntropyEncoder())
{
}