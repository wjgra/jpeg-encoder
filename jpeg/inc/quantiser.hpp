#ifndef _JPEG_QUANTISER_HPP_
#define _JPEG_QUANTISER_HPP_

#include <cmath>

#include "..\inc\discrete_cosine_transform.hpp"

namespace jpeg{

    struct QuantisedChannelOutput{
        std::array<int16_t, BlockGrid::blockElements> data;
    };

    class Quantiser{
    public:
        Quantiser(int quality = 50);
        QuantisedChannelOutput quantise(DCTChannelOutput const& dctInput, bool useLuminanceMatrix) const;
        DCTChannelOutput dequantise(QuantisedChannelOutput const& quantisedInput, bool useLuminanceMatrix) const;
    private:
        std::array<uint16_t, BlockGrid::blockElements> luminanceQuantisationMatrix;
        std::array<uint16_t, BlockGrid::blockElements> chrominanceQuantisationMatrix;
    };
}
#endif