#ifndef _JPEG_QUANTISER_HPP_
#define _JPEG_QUANTISER_HPP_

#include <cmath>

#include "discrete_cosine_transform.hpp"
#include "bit_stream.hpp"
#include "markers.hpp"

namespace jpeg{

    struct QuantisedBlockChannelData{
        std::array<int16_t, BlockGrid::blockElements> m_data;
    };

    class Quantiser{
    public:
        Quantiser(int quality = 50);
        QuantisedBlockChannelData quantise(DctBlockChannelData const& dctInput, bool useLuminanceMatrix) const;
        DctBlockChannelData dequantise(QuantisedBlockChannelData const& quantisedInput, bool useLuminanceMatrix) const;
        void encodeHeaderQuantisationTables(BitStream& outputStream) const;
        /* Issue: include decoding for non-default tables */
    private:
        std::array<uint16_t, BlockGrid::blockElements> m_luminanceQuantisationMatrix;
        std::array<uint16_t, BlockGrid::blockElements> m_chrominanceQuantisationMatrix;
    };
}
#endif