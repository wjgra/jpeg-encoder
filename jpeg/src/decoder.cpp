#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder) : bitmapImageData(inputImage.width, inputImage.height)   
{
    // bitmapImageData = BitmapImageRGB(8,8);
    //bitmapImageData.data = std::vector<BitmapImageRGB::PixelData>(64, {1, 2, 3});
    BlockGrid blockGrid(bitmapImageData); // change param to non-const reference, add both const and non const iterators

    auto temp = blockGrid.begin();
    auto temp2 = blockGrid.end();
    for (auto block : blockGrid){
        
        // 
    }
}

/* void jpeg::Decoder::saveBitmapToFile(std::string const& savePath){
    // translate to binary, save to path
    std::cout << savePath << "\n"; // temp
} */

jpeg::BitmapImageRGB jpeg::Decoder::getBitmapImageData(){
    return bitmapImageData;
}

jpeg::JPEGDecoder::JPEGDecoder(JPEGImage const& inputImage, int quality) : 
    Decoder(inputImage, RGBToYCbCrMapper(), NaiveDCTTransformer(), Quantiser(quality), HuffmanEncoder())
{
}