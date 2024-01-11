#ifndef _JPEG_QUANTISER_HPP_
#define _JPEG_QUANTISER_HPP_

#include "..\inc\discrete_cosine_transform.hpp"

#include <cmath>

namespace jpeg{

    struct QuantisedChannelOutput{
        std::array<int16_t, BlockGrid::blockSize * BlockGrid::blockSize> data;
    };

    class Quantiser{
    public:
        Quantiser(int quality = 50);
        QuantisedChannelOutput quantise(DCTChannelOutput const& dctInput) const;
    private:
        std::array<uint8_t, BlockGrid::blockSize * BlockGrid::blockSize> quantisationMatrix; // set in ctor?
    };
}
#endif