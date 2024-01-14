#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder) : bitmapImageData(inputImage.width, inputImage.height)   
{
    OutputBlockGrid outputBlockGrid(inputImage.width, inputImage.height);
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    for (auto const& blockData : inputImage.data){
        ColourMappedBlock thisBlock;
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            EntropyChannelOutput entropyEncodedData; // To do: extract enttopy encoded data from bitstream
            entropyEncodedData.temp = blockData.components[channel].tempRLE; // Temporary means of recovering entropy encoded data
            QuantisedChannelOutput quantisedData = entropyEncoder.decode(entropyEncodedData, lastDCValues[channel]); // Currently no Huffman encoding/decoding
            DCTChannelOutput dctData = quantiser.dequantise(quantisedData);
            ColourMappedBlock::ChannelBlock colourMappedChannelData = discreteCosineTransformer.inverseTransform(dctData);
            thisBlock.data[channel] = colourMappedChannelData;
        }
       outputBlockGrid.processNextBlock(colourMapper.unmap(thisBlock));
    }
    bitmapImageData = outputBlockGrid.getBitmapRGB();
}

/* void jpeg::Decoder::saveBitmapToFile(std::string const& savePath){
    // translate to binary, save to path
    std::cout << savePath << "\n"; // temp
} */

jpeg::BitmapImageRGB jpeg::Decoder::getBitmapImageData(){
    return bitmapImageData;
}

jpeg::JPEGDecoder::JPEGDecoder(JPEGImage const& inputImage, int quality) : 
    Decoder(inputImage, RGBToRGBMapper()/* RGBToYCbCrMapper() */, NaiveDCTTransformer(), Quantiser(quality), HuffmanEncoder())
{
}