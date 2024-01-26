#include "..\inc\encoder.hpp"

#include <chrono>

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       JPEGImage& outputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* OPTIONAL: sequential/progressive option; downsampling option */
{
    // reset bit stream
    outputImage.bitData.clearStream();
    // TBC: write header to bitstream

    outputImage.data.clear();
    InputBlockGrid blockGrid(inputImage);
    std::array<int16_t, 3> lastDCValues = {0,0,0};

    for (auto const& block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        outputImage.data.emplace_back(); // new encoded block
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(colourMappedBlock.data[channel]);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);
            EntropyChannelOutput entropyCodedOutput = entropyEncoder.encode(quantisedOutput, lastDCValues[channel], outputImage.bitData);
            outputImage.data.back().components[channel].tempRLE = entropyCodedOutput.temp; // to be deleted!
        }
    }
    outputImage.width = inputImage.width;
    outputImage.height = inputImage.height;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage, JPEGImage& outputImage, int quality) : 
    Encoder(inputImage, outputImage, RGBToRGBMapper(), SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}