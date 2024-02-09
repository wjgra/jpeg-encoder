#ifndef _JPEG_COLOUR_MAPPING_HPP_
#define _JPEG_COLOUR_MAPPING_HPP_

#include <array>
#include <cmath>
#include <cassert>

#include "bitmap_image.hpp"
#include "block_grid.hpp"

namespace jpeg{
    struct ColourMappedBlockData{
        using BlockChannelData = std::array<uint8_t, BlockGrid::blockElements>;
        std::array<BlockChannelData, 3> m_data;
    };
    class ColourMapper{
    public:
        ColourMapper() = default;
        ColourMapper(ColourMapper const&) = delete;
        ColourMapper(ColourMapper const&&) = delete;
        ColourMapper& operator=(ColourMapper const&) = delete;
        ColourMapper& operator=(ColourMapper const&&) = delete;
        virtual ~ColourMapper() = default;
    public:
        ColourMappedBlockData map(BlockGrid::Block const& inputBlock) const;
        BlockGrid::Block unmap(ColourMappedBlockData const& inputBlock) const;
        bool isLuminanceComponent(uint8_t component) const;
    protected:
        virtual ColourMappedBlockData applyMapping(BlockGrid::Block const& inputBlock) const = 0;
        virtual BlockGrid::Block reverseMapping(ColourMappedBlockData const& inputBlock) const = 0;
        virtual bool componentIsLuminance(uint8_t component) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    protected:
        ColourMappedBlockData applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlockData const& inputBlock) const override;
        bool componentIsLuminance(uint8_t component) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    protected:
        ColourMappedBlockData applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlockData const& inputBlock) const override;
        bool componentIsLuminance(uint8_t component) const override;
    };
}
#endif