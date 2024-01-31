
#include "..\inc\encoder.hpp"

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       JPEGImage& outputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* Issue: implement sequential/progressive option; downsampling option */
{
    outputImage.compressedImageData.clearStream();
    encodeHeader(inputImage, outputImage.compressedImageData);
    InputBlockGrid blockGrid(inputImage);
    std::array<int16_t, 3> lastDCValues = {0,0,0};
    for (auto const& block : blockGrid){
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            DCTChannelOutput dctData = discreteCosineTransformer.transform(colourMappedBlock.data[channel]);
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData, colourMapper.isLuminanceComponent(channel));
            entropyEncoder.encode(quantisedOutput, lastDCValues[channel], outputImage.compressedImageData, colourMapper.isLuminanceComponent(channel));
        }
    }
    // Push end of image marker
    outputImage.compressedImageData.pushIntoAlignment();
    outputImage.compressedImageData.pushWord(markerEndOfImageSegmentEOI);

    outputImage.width = inputImage.width;
    outputImage.height = inputImage.height;
    outputImage.fileSize = outputImage.compressedImageData.getSize();
}

/* Issue: currently hardcoded with baseline parameters*/
void jpeg::Encoder::encodeHeader(BitmapImageRGB const& inputImage, BitStream& outputStream) const {
    // SOI
    outputStream.pushWord(markerStartOfImageSegmentSOI);
    // APP-0
    outputStream.pushWord(markerJFIFImageSegmentAPP0);
    outputStream.pushWord(0x0010); // length
    outputStream.pushByte('J');
    outputStream.pushByte('F');
    outputStream.pushByte('I');
    outputStream.pushByte('F');
    outputStream.pushByte(00);
    outputStream.pushWord(0x0102); // version
    outputStream.pushByte(0x00); // density units
    outputStream.pushWord(0x0001); // Xdensity
    outputStream.pushWord(0x0001); // Ydensity
    outputStream.pushWord(0x0000); // thumbnail size
    // COM

    // DQT

    // SOF0
    outputStream.pushWord(markerStartOfFrame0SOF0);
    /* length */

    // DHT

    // SOS 

}

jpeg::BaselineEncoder::BaselineEncoder(BitmapImageRGB const& inputImage, JPEGImage& outputImage, int quality) : 
    Encoder(inputImage, outputImage, RGBToYCbCrMapper(), SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
    outputImage.supportsSaving = true;
}