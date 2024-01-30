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
    outputImage.compressedImageData.clearStream();
    // TBC: write header to bitstream

    InputBlockGrid blockGrid(inputImage);
    std::array<int16_t, 3> lastDCValues = {0,0,0};

    for (auto const& block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(colourMappedBlock.data[channel]);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);
            entropyEncoder.encode(quantisedOutput, lastDCValues[channel], outputImage.compressedImageData);
        }
    }
    outputImage.width = inputImage.width;
    outputImage.height = inputImage.height;
    outputImage.fileSize = outputImage.compressedImageData.getSize(); // temp - need to include header
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage, JPEGImage& outputImage, int quality) : 
    Encoder(inputImage, outputImage, RGBToYCbCrMapper(), SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}