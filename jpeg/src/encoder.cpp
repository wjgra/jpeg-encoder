#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* OPTIONAL: sequential/progressive option; downsampling option */
{
    jpegImageData.data.clear();
    BlockGrid blockGrid(inputImage);
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    for (auto const& block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        /* 
        OPTIONAL : downsampling! Recall this is 'the point' of using YCbCr
        Note that components are processed separately
        Consider having modules act on components?
         */
        jpegImageData.data.emplace_back(); // new encoded block
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(colourMappedBlock.data[channel]);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);

            EntropyChannelOutput entropyCodedOutput = entropyEncoder.encode(quantisedOutput, lastDCValues[channel]);
            // **Temporary** DC diff should be set by encoder...
            /* entropyCodedOutput.temp.dcDifference = quantisedOutput.data[0] - lastDCValues[channel];
            lastDCValues[channel] = quantisedOutput.data[0]; */
            jpegImageData.data.back().components[channel].temp = entropyCodedOutput.temp;
            /* To do: temp (vector) should be replaced by bitstream*/
        }

        // Push to stream
        // jpegImageData.data.push_back(encodedBlock);
    }
    // Assign compressed data to struct
    jpegImageData.width = inputImage.width;
    jpegImageData.height = inputImage.height;
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