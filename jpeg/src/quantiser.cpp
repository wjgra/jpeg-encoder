#include "..\inc\quantiser.hpp"

jpeg::Quantiser::Quantiser(int quality){
    /* Generates a quantisation matrix of a given quality, as described in 
    this SO answer https://stackoverflow.com/a/29216609 */
    if (quality < 1){
        quality = 1;
    }
    else if (quality > 100){
        quality = 100;
    }
    
    /* Base luminance quantisation matrix as defined in Annex K of ITU T81 */ 
    std::array<uint16_t const, BlockGrid::blockElements> baseLuminanceMatrix =
    {
        16,  11,  10,  16,  24,  40,  51,  61,
        12,  12,  14,  19,  26,  58,  60,  55,
        14,  13,  16,  24,  40,  57,  69,  56,
        14,  17,  22,  29,  51,  87,  80,  62,
        18,  22,  37,  56,  68,  109, 103, 77,
        24,  35,  55,  64,  81,  104, 113, 92,
        49,  64,  78,  87,  103, 121, 120, 101,
        72,  92,  95,  98,  112, 100, 103, 99
    };
    float S = (quality < 50) ? 5000/quality : 200 - 2 * quality;
    for (size_t i = 0 ; i < luminanceQuantisationMatrix.size() ; ++i){
        luminanceQuantisationMatrix[i] = std::floor(
            (S * baseLuminanceMatrix[i] + 50) / 100
        );
        if (luminanceQuantisationMatrix[i] == 0){
            luminanceQuantisationMatrix[i] = 1;
        }
    }

    /* Base chrominance quantisation matrix as defined in Annex K of ITU T81 */ 
    std::array<uint16_t const, BlockGrid::blockElements> baseChrominanceMatrix =
    {
        17,  18,  24,  47,  99,  99,  99,  99,
        18,  21,  26,  66,  99,  99,  99,  99,
        24,  26,  56,  99,  99,  99,  99,  99,
        47,  66,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99,
        99,  99,  99,  99,  99,  99,  99,  99
    };
    for (size_t i = 0 ; i < chrominanceQuantisationMatrix.size() ; ++i){
        chrominanceQuantisationMatrix[i] = std::floor(
            (S * baseChrominanceMatrix[i] + 50) / 100
        );
        if (chrominanceQuantisationMatrix[i] == 0){
            chrominanceQuantisationMatrix[i] = 1;
        }
    }
}

jpeg::QuantisedChannelOutput jpeg::Quantiser::quantise(DCTChannelOutput const& dctInput, bool useLuminanceMatrix) const{
    QuantisedChannelOutput output;
    if (useLuminanceMatrix){
        for (size_t i = 0 ; i < dctInput.data.size() ; ++i){
            output.data[i] = std::floor(0.5 + dctInput.data[i]/luminanceQuantisationMatrix[i]);
        }
    }
    else{
        for (size_t i = 0 ; i < dctInput.data.size() ; ++i){
            output.data[i] = std::floor(0.5 + dctInput.data[i]/chrominanceQuantisationMatrix[i]);
        }
    }
    return output;
}

jpeg::DCTChannelOutput jpeg::Quantiser::dequantise(jpeg::QuantisedChannelOutput const& quantisedChannelData, bool useLuminanceMatrix) const{
    DCTChannelOutput output;
    if (useLuminanceMatrix){
        for (size_t i = 0 ; i < quantisedChannelData.data.size() ; ++i){
            output.data[i] = quantisedChannelData.data[i] * float(luminanceQuantisationMatrix[i]);
        }
    }
    else{
        for (size_t i = 0 ; i < quantisedChannelData.data.size() ; ++i){
            output.data[i] = quantisedChannelData.data[i] * float(chrominanceQuantisationMatrix[i]);
        }
    }
    return output;
}

void jpeg::Quantiser::encodeHeaderQuantisationTables(BitStream& outputStream) const{
    outputStream.pushWord(markerDefineQuantisationTableSegmentDQT);
    outputStream.pushWord(2 + 2 * 65); // len
    outputStream.pushByte(0x00); // precision + table ID
    /* Temporary hardcoded zigzag indices - extract zig zag method from entropyencoder for use here */
    std::array<uint8_t, BlockGrid::blockElements> zigZagIndices{0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63};
    for (auto const& index : zigZagIndices){
        outputStream.pushByte(luminanceQuantisationMatrix[index]);
    }
    /* for (auto& element : luminanceQuantisationMatrix){
        outputStream.pushByte(element);
    } */
    outputStream.pushByte(0x01); // precision + table ID
    /* for (auto& element : chrominanceQuantisationMatrix){
        outputStream.pushByte(element);
    } */
    for (auto const& index : zigZagIndices){
        outputStream.pushByte(chrominanceQuantisationMatrix[index]);
    }
}