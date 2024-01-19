#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                       BitmapImageRGB& outputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder) /* : bitmapImageData(inputImage.width, inputImage.height) */   
{
    OutputBlockGrid outputBlockGrid(inputImage.width, inputImage.height);
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    for (auto const& blockData : inputImage.data){
        ColourMappedBlock thisBlock;
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            EntropyChannelOutput entropyEncodedData; // To do: extract entropy encoded data from bitstream
            entropyEncodedData.temp = blockData.components[channel].tempRLE; // Temporary means of recovering entropy encoded data
            QuantisedChannelOutput quantisedData = entropyEncoder.decode(entropyEncodedData, lastDCValues[channel]); // Currently no Huffman encoding/decoding
            DCTChannelOutput dctData = quantiser.dequantise(quantisedData);
            ColourMappedBlock::ChannelBlock colourMappedChannelData = discreteCosineTransformer.inverseTransform(dctData);
            thisBlock.data[channel] = colourMappedChannelData;
        }
       outputBlockGrid.processNextBlock(colourMapper.unmap(thisBlock));
    }
    outputImage = outputBlockGrid.getBitmapRGB();
}

jpeg::JPEGDecoder::JPEGDecoder(JPEGImage const& inputImage, BitmapImageRGB& outputImage, int quality) : 
    Decoder(inputImage, outputImage, RGBToRGBMapper()/* RGBToYCbCrMapper() */, SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}