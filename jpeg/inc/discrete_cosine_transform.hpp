#ifndef _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_
#define _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_

#include <array>
#include <span>
#include <cmath>
#include <numbers>

#include "colour_mapping.hpp"
#include "block_grid.hpp"
namespace jpeg{

    struct DCTChannelOutput{
            std::array<float, BlockGrid::blockElements> data;
            float getDCCoefficient(){return data.front();}
    };

    class DiscreteCosineTransformer{
    public:
        virtual ~DiscreteCosineTransformer() = default;
        DCTChannelOutput transform(ColourMappedBlock::ChannelBlock const& inputChannel) const;
        ColourMappedBlock::ChannelBlock inverseTransform(DCTChannelOutput  const& inputChannel) const;
    protected:
        std::array<int8_t, BlockGrid::blockElements> applyOffset(ColourMappedBlock::ChannelBlock const& input) const;
        virtual DCTChannelOutput applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const = 0;
        ColourMappedBlock::ChannelBlock removeOffset(std::array<int8_t, BlockGrid::blockElements> const& input) const;
        virtual std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DCTChannelOutput  const& inputChannel) const = 0;
    };

    /* Calculates the 2D DCT/IDCT by direct calculation in O(blockSize^4). */
    class NaiveCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const override;
        std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    };

    /* Exploits separability of the 2D DCT/IDCT to split calculation into row and column transforms,
       thereby reducing complexity to O(blockSize^3). Further speedup may be achieved by using a more 
       efficient 1D transformer. */
    class SeparatedDiscreteCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DCTChannelOutput applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const override;
        std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DCTChannelOutput  const& inputChannel) const override;
    private:
        void apply1DTransformRow(int8_t const* src, float* dest, uint8_t u) const;
        void apply1DTransformCol(float const* src, float* dest, uint8_t v) const;
        void apply1DInverseTransformRow(float const* src, float* dest, uint8_t x) const;
        void apply1DInverseTransformCol(float const* src, int8_t* dest, uint8_t y) const;
    };    
}
#endif