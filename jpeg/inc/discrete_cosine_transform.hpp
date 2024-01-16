#ifndef _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_
#define _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_

#include <array>
#include <span>
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

    /* Calculates the 2D DCT/IDCT by direct calculation. */
    class NaiveCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const override;
        ColourMappedBlock::ChannelBlock applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    };

    /* Calculates the 2D DCT by applying the 1D transform recursively. 
       While this is quicker than the naive transformer in un-optimised builds, the naive transformer is more effectively
       optimised in -O3 builds (naive is ~3x faster!). */
    class NestedCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const override;
        ColourMappedBlock::ChannelBlock applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    private:
        std::array<float, BlockGrid::blockSize> apply1DTransform(std::array<int8_t, BlockGrid::blockSize> const& input) const;
    };
}
#endif