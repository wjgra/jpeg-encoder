#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                       BitmapImageRGB& outputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder) /* : bitmapImageData(inputImage.width, inputImage.height) */   
{
    OutputBlockGrid outputBlockGrid(inputImage.width, inputImage.height);
    // Decoding status variables
    BitStreamReadProgress readProgress{};
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    // Decode image block-by-block
    while (!outputBlockGrid.atEnd()){
        ColourMappedBlock thisBlock;
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            QuantisedChannelOutput quantisedData = entropyEncoder.decode(inputImage.compressedImageData, readProgress, lastDCValues[channel]);
            DCTChannelOutput dctData = quantiser.dequantise(quantisedData, colourMapper.isLuminanceComponent(channel));
            ColourMappedBlock::ChannelBlock colourMappedChannelData = discreteCosineTransformer.inverseTransform(dctData);
            thisBlock.data[channel] = colourMappedChannelData;
        }
       outputBlockGrid.processNextBlock(colourMapper.unmap(thisBlock));
    }
    outputImage = outputBlockGrid.getBitmapRGB();
}

jpeg::JPEGDecoder::JPEGDecoder(JPEGImage const& inputImage, BitmapImageRGB& outputImage, int quality) : 
    Decoder(inputImage, outputImage, RGBToYCbCrMapper(), SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}