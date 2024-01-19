#include "..\inc\encoder.hpp"

#include <chrono>

jpeg::Encoder::Encoder(BitmapImageRGB const& inputImage,
                       ColourMapper const& colourMapper,
                       DiscreteCosineTransformer const& discreteCosineTransformer,
                       Quantiser const& quantiser,
                       EntropyEncoder const& entropyEncoder)
                       /* OPTIONAL: sequential/progressive option; downsampling option */
{
    jpegImageData.data.clear();
    InputBlockGrid blockGrid(inputImage);
    std::array<int16_t, 3> lastDCValues = {0,0,0};

    int mappingTime = 0;
    int dctTime = 0;
    int entropyTime = 0;
    int quantisingTime = 0;

    for (auto const& block : blockGrid){
        auto t1 = std::chrono::high_resolution_clock::now();
        ColourMappedBlock colourMappedBlock = colourMapper.map(block);
        auto t2 = std::chrono::high_resolution_clock::now();
        mappingTime += std::chrono::duration_cast<std::chrono::microseconds>((t2-t1)).count();
        /* 
        OPTIONAL : downsampling! Recall this is 'the point' of using YCbCr
        Note that components are processed separately
        Consider having modules act on components?
         */
        jpegImageData.data.emplace_back(); // new encoded block
        for (size_t channel = 0 ; channel < 3 ; ++channel){
            t2 = std::chrono::high_resolution_clock::now();
            DCTChannelOutput dctData = discreteCosineTransformer.transform(colourMappedBlock.data[channel]);
            auto t3 = std::chrono::high_resolution_clock::now();
            dctTime += std::chrono::duration_cast<std::chrono::microseconds>((t3-t2)).count();
            
            QuantisedChannelOutput quantisedOutput = quantiser.quantise(dctData);
            auto t4 = std::chrono::high_resolution_clock::now();
            quantisingTime += std::chrono::duration_cast<std::chrono::microseconds>((t4-t3)).count();

            EntropyChannelOutput entropyCodedOutput = entropyEncoder.encode(quantisedOutput, lastDCValues[channel]);
            auto t5 = std::chrono::high_resolution_clock::now();
            entropyTime += std::chrono::duration_cast<std::chrono::microseconds>((t5-t4)).count();

            jpegImageData.data.back().components[channel].tempRLE = entropyCodedOutput.temp;
            /* To do: temp (vector) should be replaced by bitstream*/

            // temp - for use in short-circuiting the decoder
            jpegImageData.data.back().components[channel].tempColMapBlock = colourMappedBlock;
            jpegImageData.data.back().components[channel].tempDCT = dctData;
            jpegImageData.data.back().components[channel].tempQuantised = quantisedOutput;
        }

        // Push to stream
        // jpegImageData.data.push_back(encodedBlock);
    }
    // Assign compressed data to struct
    jpegImageData.width = inputImage.width;
    jpegImageData.height = inputImage.height;
    std::cout << "Mapping: " << mappingTime << "\n";
    std::cout << "DCT: " << dctTime << "\n";
    std::cout << "Quantisation: " << quantisingTime << "\n";
    std::cout << "Entropy coding: " << entropyTime << "\n";
    std::cout << "Compression ratio: 1.0" << "\n";
}

/* void jpeg::Encoder::saveJPEGToFile(std::string const& savePath){
    // translate to binary, save to path
    std::cout << savePath << "\n"; // temp
} */

jpeg::JPEGImage jpeg::Encoder::getJPEGImageData(){
    return jpegImageData;
}

jpeg::JPEGEncoder::JPEGEncoder(BitmapImageRGB const& inputImage, int quality) : 
    Encoder(inputImage, RGBToRGBMapper()/* RGBToYCbCrMapper() */, SeparatedDiscreteCosineTransformer(), Quantiser(quality), HuffmanEncoder())
{
}