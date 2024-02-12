#ifndef _JPEG_BLOCK_GRID_HPP_
#define _JPEG_BLOCK_GRID_HPP_

#include <cstdint>
#include <cstring>
#include <array>
#include <iterator>

#include "bitmap_image.hpp"

namespace jpeg{

class BlockGrid{
protected:
    BlockGrid() = default; // Prevent instantiation
public:
    uint8_t const static blockSize = 8; // Issue: include support for downsampling (e.g. via 16x16 blocks)
    uint8_t const static blockElements = blockSize * blockSize;
    struct Block{
        std::array<BitmapImageRGB::PixelData, blockElements> m_blockPixelData;
    };
};
class InputBlockGrid : public BlockGrid{
    public:
        InputBlockGrid(BitmapImageRGB const& input);
        // Input iterator for accessing image blocks
        struct BlockIterator{
            using difference_type = std::ptrdiff_t;
            using value_type = BlockGrid::Block;
            using underlying_type = BitmapImageRGB::PixelData;
            using underlying_pointer = underlying_type const *;
            using iterator_category = std::input_iterator_tag;
        private:
            underlying_pointer m_ptr; // Points to first (i.e. upper-left) pixel in block
            uint16_t m_blockRowPos, m_blockColPos; // Position in block-rows and block-columns
            uint16_t m_gridWidth, m_gridHeight;
        public:
            explicit BlockIterator() = default;
            BlockIterator(underlying_pointer p, uint16_t w, uint16_t h);
            value_type operator*() const;
            BlockIterator& operator++();
            BlockIterator operator++(int);
            friend bool operator==(BlockIterator const& a, BlockIterator const& b){return a.m_ptr == b.m_ptr;}
            friend bool operator!=(BlockIterator const& a, BlockIterator const& b){return a.m_ptr != b.m_ptr;}
            bool isLastCol() const;
            bool isLastRow() const;
            underlying_pointer getDataPtr() const;
        };
        static_assert(std::input_iterator<BlockIterator>);
        
    public:
        BlockIterator begin() const;
        BlockIterator end() const;
    private:
        BitmapImageRGB const& m_imageData;
    };

    class OutputBlockGrid : public BlockGrid{
    private:
        BitmapImageRGB m_output;
        InputBlockGrid m_blockGrid;
        InputBlockGrid::BlockIterator m_currentBlock;
        uint16_t const m_gridWidth, m_gridHeight;
    public:
        OutputBlockGrid() = delete;
        OutputBlockGrid(uint16_t width, uint16_t height);
        void processNextBlock(BlockGrid::Block const& inputBlock);
        BitmapImageRGB getBitmapRGB() const;
        bool atEnd() const;
    private:
        BitmapImageRGB::PixelData* getBlockPtr();
    };
}
#endif