#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* OPTIONAL: sequential/progressive option; downsampling option */
{
    BlockGrid blockGrid(inputImage);
    // init entropy encoder - set dc diff for each channel to zero
    jpegImageData.data.clear();
    for (auto block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        /* 
        OPTIONAL : downsampling! Recall this is 'the point' of using YCbCr
        Note that components are processed separately
        Consider having modules act on components?
         */
        JPEGImage::BlockData thisBlock;
        size_t chan = 0; // temp
        int16_t lastDCValue = 0; // separate this...
        for (auto channel : colourMappedBlock.data){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(channel);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);
            EntropyChannelOutput entropyCodedOutput = entropyEncoder.encode(quantisedOutput);
            // Push to stream
            // temp
            entropyCodedOutput.temp.dcDifference = quantisedOutput.data[0] - lastDCValue;
            lastDCValue = quantisedOutput.data[0];
            thisBlock.components[chan++].temp = entropyCodedOutput.temp;
        }
        // Push to stream
        jpegImageData.data.push_back(thisBlock);
    }
    // Assign compressed data to struct
    jpegImageData.width = inputImage.width;
    jpegImageData.height = inputImage.height;

    for (auto block : blockGrid){}
}

/* void jpeg::Encoder::saveJPEGToFile(std::string const& savePath){
    // translate to binary, save to path
    std::cout << savePath << "\n"; // temp
} */

jpeg::JPEGImage jpeg::Encoder::getJPEGImageData(){
    return jpegImageData;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage, int quality) : 
    Encoder(inputImage, /* RGBToRGBMapper() */RGBToYCbCrMapper(), NaiveDCTTransformer(), Quantiser(quality), HuffmanEncoder())
{
}