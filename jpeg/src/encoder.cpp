#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* OPTIONAL: sequential/progressive option; downsampling option */
{
    BlockGrid blockGrid(inputImage);
    for (auto block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        /* 
        OPTIONAL : downsampling! Recall this is 'the point' of using YCbCr
        Note that components are processed separately
        Consider having modules act on components?
         */
        for (auto channel : colourMappedBlock.data){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(channel);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);
            EntropyChannelOutput entropyCodedOutput = entropyEncoder.encode(quantisedOutput);
            // Push to stream
        }
        // Push to stream
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

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage, int quality) : 
    Encoder(inputImage, /* RGBToRGBMapper() */RGBToYCbCrMapper(), NaiveDCTTransformer(), Quantiser(quality), EntropyEncoder())
{
}