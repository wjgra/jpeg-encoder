#include "..\inc\decoder.hpp"

jpeg::Decoder::Decoder(JPEGImage inputImage,
                       BitmapImageRGB& outputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder) /* : bitmapImageData(inputImage.width, inputImage.height) */   
{
    OutputBlockGrid outputBlockGrid(inputImage.width, inputImage.height);
    BitStreamReadProgress readProgress{};
    decodeHeader(inputImage.compressedImageData, readProgress, outputImage);
    inputImage.compressedImageData.removeStuffedBytes(readProgress);
    // Decode image block-by-block
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    while (!outputBlockGrid.atEnd()){
        ColourMappedBlock thisBlock;
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            QuantisedChannelOutput quantisedData = entropyEncoder.decode(inputImage.compressedImageData, readProgress, lastDCValues[channel], colourMapper.isLuminanceComponent(channel));
            DCTChannelOutput dctData = quantiser.dequantise(quantisedData, colourMapper.isLuminanceComponent(channel));
            ColourMappedBlock::ChannelBlock colourMappedChannelData = discreteCosineTransformer.inverseTransform(dctData);
            thisBlock.data[channel] = colourMappedChannelData;
        }
       outputBlockGrid.processNextBlock(colourMapper.unmap(thisBlock));
    }
    // Check end of image marker
    if (inputImage.compressedImageData.readNextAlignedWord(readProgress) != markerEndOfImageSegmentEOI){
        throw "Failed to find EOI marker";
    }
    outputImage = outputBlockGrid.getBitmapRGB();
}

void jpeg::Decoder::decodeHeader(BitStream const& inputStream, BitStreamReadProgress& readProgress, BitmapImageRGB& outputImage) const{
    if (inputStream.readNextAlignedWord(readProgress) != markerStartOfImageSegmentSOI){
        throw "Failed to find SOI marker";
    }
    if (inputStream.readNextAlignedWord(readProgress) != markerJFIFImageSegmentAPP0){
        throw "Failed to find APP-0 marker";
    }
    {
        auto startOfAPP0Payload = readProgress.currentByte;
        auto APP0length = inputStream.readNextAlignedWord(readProgress);
        if (inputStream.readNextAlignedByte(readProgress) != 'J'){
            throw "Failed to find 'J'(FIF) in APP-0 payload";
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'F'){
            throw "Failed to find (J)'F'(IF) in APP-0 payload";
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'I'){
            throw "Failed to find (JF)'I'(F) in APP-0 payload";
        }
        if (inputStream.readNextAlignedByte(readProgress) != 'F'){
            throw "Failed to find (JFI)'F' in APP-0 payload";
        }
        if (inputStream.readNextAlignedByte(readProgress) != 00){
            throw "Failed to find 'JFIF' null-terminator in APP-0 payload";
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0102){
            throw "JFIF version indicated by APP-0 payload is not 1.02";
        }
        if (inputStream.readNextAlignedByte(readProgress) != 00){
            throw "Failed to find density units in APP-0 payload";
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0001){
            throw "Failed to find Xdensity in APP-0 payload";
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0001){
            throw "Failed to find Ydensity in APP-0 payload";
        }
        if (inputStream.readNextAlignedWord(readProgress) != 0x0000){
            throw "Failed to find thumbnail size in APP-0 payload";
        }
        if (APP0length != readProgress.currentByte - startOfAPP0Payload){
            throw "APP-0 length parameter does not correspond to payload size";
        }
    }

    // read next marker - switch b/w sof/com/dqt/dht

    if (inputStream.readNextAlignedWord(readProgress) != markerStartOfFrame0SOF0){
        throw "Failed to find SOF0 marker";
    }
    {

    }
}

jpeg::BaselineDecoder::BaselineDecoder(JPEGImage const& inputImage, BitmapImageRGB& outputImage, int quality) : 
    Decoder(inputImage, outputImage, RGBToYCbCrMapper(), SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}