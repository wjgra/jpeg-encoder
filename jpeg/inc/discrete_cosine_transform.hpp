#ifndef _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_
#define _JPEG_DISCRETE_COSINE_TRANSFORM_HPP_

#include <array>
#include <span>
#include <cmath>
#include <numbers>

#include "colour_mapping.hpp"
#include "block_grid.hpp"
namespace jpeg{

    /* Stores the results of performing a 2D DCT on a a single channel of a block */
    struct DctBlockChannelData{
            std::array<float, BlockGrid::blockElements> m_data;
            float getDCCoefficient(){return m_data.front();}
    };

    class DiscreteCosineTransformer{
    public:
        DiscreteCosineTransformer() = default;
        DiscreteCosineTransformer(DiscreteCosineTransformer const&) = delete;
        DiscreteCosineTransformer(DiscreteCosineTransformer const&&) = delete;
        DiscreteCosineTransformer& operator=(DiscreteCosineTransformer const&) = delete;
        DiscreteCosineTransformer& operator=(DiscreteCosineTransformer const&&) = delete;
        virtual ~DiscreteCosineTransformer() = default;
    public:
        DctBlockChannelData transform(ColourMappedBlockData::BlockChannelData const& inputChannel) const;
        ColourMappedBlockData::BlockChannelData inverseTransform(DctBlockChannelData  const& inputChannel) const;
    protected:
        std::array<int8_t, BlockGrid::blockElements> applyOffset(ColourMappedBlockData::BlockChannelData const& input) const;
        virtual DctBlockChannelData applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const = 0;
        ColourMappedBlockData::BlockChannelData removeOffset(std::array<int8_t, BlockGrid::blockElements> const& input) const;
        virtual std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DctBlockChannelData  const& inputChannel) const = 0;
    };

    /* Calculates the 2D DCT/IDCT by direct calculation in O(blockSize^4). */
    class NaiveCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DctBlockChannelData applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const override;
        std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DctBlockChannelData  const& inputChannel) const override;
    };

    /* Exploits separability of the 2D DCT/IDCT to split calculation into row and column transforms,
       thereby reducing complexity to O(blockSize^3). Further speedup may be achieved by using a more 
       efficient 1D transformer. */
    class SeparatedDiscreteCosineTransformer : public DiscreteCosineTransformer{
    protected:
        DctBlockChannelData applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const override;
        std::array<int8_t, BlockGrid::blockElements> applyInverseTransform(DctBlockChannelData  const& inputChannel) const override;
    private:
        void apply1DTransformRow(int8_t const* src, float* dest, uint8_t u) const;
        void apply1DTransformCol(float const* src, float* dest, uint8_t v) const;
        void apply1DInverseTransformRow(float const* src, float* dest, uint8_t x) const;
        void apply1DInverseTransformCol(float const* src, int8_t* dest, uint8_t y) const;
    };    
}
#endif