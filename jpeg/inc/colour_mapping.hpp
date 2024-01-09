#ifndef _JPEG_COLOUR_MAPPING_HPP_
#define _JPEG_COLOUR_MAPPING_HPP_

#include "..\inc\bitmap_image.hpp"
#include "..\inc\block_grid.hpp"

#include <array>
#include <vector>

namespace jpeg{
    struct ColourMappedBlock{
        // std::array<std::array<uint8_t, 3>, jpeg::BlockGrid::blockSize * jpeg::BlockGrid::blockSize> data;
        using ChannelBlock = std::array<uint8_t, jpeg::BlockGrid::blockSize * jpeg::BlockGrid::blockSize>;
        std::array<ChannelBlock, 3> data;
    };
    class ColourMapper{
    public:
        ColourMappedBlock map(jpeg::BlockGrid::Block const& inputBlock) const;
    protected:
        virtual ColourMappedBlock applyMapping(jpeg::BlockGrid::Block const& inputBlock) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    protected:
        virtual ColourMappedBlock applyMapping(jpeg::BlockGrid::Block const& inputBlock) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    protected:
        virtual ColourMappedBlock applyMapping(jpeg::BlockGrid::Block const& inputBlock) const override;
    };
}
#endif