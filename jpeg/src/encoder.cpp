#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                    ColourMapper const& colourMapper,
                    DiscreteCosineTransformer const& discreteCosineTransform,
                    Quantiser const& quantisation,
                    EntropyEncoder const& entropyEncoder)
{
    // auto colourMappedImageData = colourMapper.map(inputImage);

    // change colour mapper to act on blocks
    // BitmapImageRGB testImage(8,8);
    BlockGrid blockGrid(inputImage);
    int i = 0, j=0;
    // Issue: merge row and block iterators, so a single range-for loop can be used
    for (auto rowIt = blockGrid.begin() ; rowIt != blockGrid.end() ; ++rowIt){
        for (auto blockIt = blockGrid.beginRow(rowIt) ; blockIt != blockGrid.endRow(rowIt) ; ++blockIt){
            // std::cout << i << ", " << j++ << "\n";
        }
        ++i;j=0;
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