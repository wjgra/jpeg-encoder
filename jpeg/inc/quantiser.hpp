#ifndef _JPEG_QUANTISER_HPP_
#define _JPEG_QUANTISER_HPP_

#include <cmath>

#include "discrete_cosine_transform.hpp"
#include "bitstream.hpp"
#include "markers.hpp"

namespace jpeg{

    struct QuantisedChannelOutput{
        std::array<int16_t, BlockGrid::blockElements> data;
    };

    class Quantiser{
    public:
        Quantiser(int quality = 50);
        QuantisedChannelOutput quantise(DCTChannelOutput const& dctInput, bool useLuminanceMatrix) const;
        DCTChannelOutput dequantise(QuantisedChannelOutput const& quantisedInput, bool useLuminanceMatrix) const;
        void encodeHeaderQuantisationTables(BitStream& outputStream) const;
        /* Issue: include decoding for non-default tables */
    private:
        std::array<uint16_t, BlockGrid::blockElements> luminanceQuantisationMatrix;
        std::array<uint16_t, BlockGrid::blockElements> chrominanceQuantisationMatrix;
    };
}
#endif