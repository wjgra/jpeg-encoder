#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder) : bitmapImageData(inputImage.width, inputImage.height)   
{
    std::vector<BlockGrid::Block> decodedBlocks;
    for (auto const& blockData : inputImage.data){
        
        ColourMappedBlock thisBlock;
        for (size_t channel = 0 ; channel < 3 ; ++channel/* auto const& channelScanData : blockData.components  */){
            // To do: extract enoded data from bitstream
            EntropyChannelOutput entropyEncodedData;
            entropyEncodedData.temp = blockData.components[channel].temp; // Temporary formulation
            QuantisedChannelOutput quantisedData = entropyEncoder.decode(entropyEncodedData); // = un-entropy encode (Huff, RLE, zig) 
            DCTChannelOutput dctData = quantiser.dequantise(quantisedData); // undo quantisation
            ColourMappedBlock::ChannelBlock colourMappedChannelData = discreteCosineTransformer.inverseTransform(dctData);// undo DCT
            thisBlock.data[channel] = colourMappedChannelData; //add to block
        }

        decodedBlocks.emplace_back(colourMapper.unmap(thisBlock)); // undo colour map and add to vector
    }
    /* 
    --Convert array of blocks to Bitmap data
    May be no need to store decoded blocks in vector - consider using iterator as you go along to get row/col number
    then map data to bitmap array using rol/col number
    any additional functions can be in blockgrid, may be overlap with dereference operator
    Blockgrid should be modified first to not use const ref
    */
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