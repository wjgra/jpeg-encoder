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
    // Base IJG quantisation matrix
    std::array<uint8_t const, BlockGrid::blockElements> baseMatrix =
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
    for (size_t i = 0 ; i < quantisationMatrix.size() ; ++i){
        quantisationMatrix[i] = std::floor(
            (S * baseMatrix[i] + 50) / 100
        );
        if (quantisationMatrix[i] == 0){
            quantisationMatrix[i] = 1;
        }
    }
}

jpeg::QuantisedChannelOutput jpeg::Quantiser::quantise(DCTChannelOutput const& dctInput) const{
    QuantisedChannelOutput output;
    for (size_t i = 0 ; i < dctInput.data.size() ; ++i){
        output.data[i] = std::floor(0.5 + dctInput.data[i]/quantisationMatrix[i]);
    }
    return output;
}

jpeg::DCTChannelOutput jpeg::Quantiser::dequantise(jpeg::QuantisedChannelOutput const& quantisedChannelData) const{
    DCTChannelOutput output;
    for (size_t i = 0 ; i < quantisedChannelData.data.size() ; ++i){
        output.data[i] = quantisedChannelData.data[i] * float(quantisationMatrix[i]);
    }
    return output;
}