#ifndef _JPEG_COLOUR_MAPPING_HPP_
#define _JPEG_COLOUR_MAPPING_HPP_

#include <array>
#include <cmath>
#include <cassert>

#include "bitmap_image.hpp"
#include "block_grid.hpp"


namespace jpeg{
    struct ColourMappedBlock{
        using ChannelBlock = std::array<uint8_t, BlockGrid::blockElements>;
        std::array<ChannelBlock, 3> data;
    };
    class ColourMapper{
    public:
        ColourMappedBlock map(BlockGrid::Block const& inputBlock) const;
        BlockGrid::Block unmap(ColourMappedBlock const& inputBlock) const;
        bool isLuminanceComponent(uint8_t component) const;
    protected:
        virtual ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const = 0;
        virtual BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const = 0;
        virtual bool componentIsLuminance(uint8_t component) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    protected:
        ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const override;
        bool componentIsLuminance(uint8_t component) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    protected:
        ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const override;
        bool componentIsLuminance(uint8_t component) const override;
    };
}
#endif