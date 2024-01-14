#ifndef _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_
#define _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_

#include <array>
#include <cmath>
#include <numbers>

#include "..\inc\colour_mapping.hpp"
#include "..\inc\block_grid.hpp" // Issue: separate dependencies of block and block grid
namespace jpeg{

    struct DCTChannelOutput{
            std::array<float, BlockGrid::blockElements> data;
            float getDCCoefficient(){return data.front();}
    };

    class DiscreteCosineTransformer{
    public:
        DCTChannelOutput transform(ColourMappedBlock::ChannelBlock const& inputChannel) const;
        ColourMappedBlock::ChannelBlock inverseTransform(DCTChannelOutput  const& inputChannel) const;
    protected:
        virtual DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const = 0;
        virtual ColourMappedBlock::ChannelBlock applyInverseTransform(DCTChannelOutput  const& inputChannel) const = 0;
    };

    /* Very slowly calculates the DCT/IDCT by direct calculation (in the naive manner) */
    class NaiveCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const override;
        ColourMappedBlock::ChannelBlock applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    };

    class FastCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const override;
        ColourMappedBlock::ChannelBlock applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    };

}
#endif