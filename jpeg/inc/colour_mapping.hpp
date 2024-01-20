#ifndef _JPEG_COLOUR_MAPPING_HPP_
#define _JPEG_COLOUR_MAPPING_HPP_

#include "..\inc\bitmap_image.hpp"
#include "..\inc\block_grid.hpp"

#include <array>
#include <cmath>

namespace jpeg{
    struct ColourMappedBlock{
        using ChannelBlock = std::array<uint8_t, BlockGrid::blockElements>;
        std::array<ChannelBlock, 3> data;
    };
    class ColourMapper{
    public:
        ColourMappedBlock map(BlockGrid::Block const& inputBlock) const;
        BlockGrid::Block unmap(ColourMappedBlock const& inputBlock) const;
    protected:
        virtual ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const = 0;
        virtual BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    protected:
        ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    protected:
        ColourMappedBlock applyMapping(BlockGrid::Block const& inputBlock) const override;
        BlockGrid::Block reverseMapping(ColourMappedBlock const& inputBlock) const override;
    };
}
#endif