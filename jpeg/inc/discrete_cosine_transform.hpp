#ifndef _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_
#define _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_

#include "..\inc\colour_mapping.hpp"
#include "..\inc\block_grid.hpp" // Issue: separate dependencies of block and block grid

#include <array>

namespace jpeg{

    struct DCTChannelOutput{
            std::array<float, BlockGrid::blockSize * BlockGrid::blockSize> data;
            float getDCCoefficient(){return data.front();}
    };

    class DiscreteCosineTransformer{
    public:
    DCTChannelOutput transform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
        return applyTransform(inputChannel);
    };
    protected:
        virtual DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const = 0;
    };

    class NaiveDCTTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const override{
            return DCTChannelOutput(); // to implement
        };
    };

    /* Add faster transformer */

}
#endif