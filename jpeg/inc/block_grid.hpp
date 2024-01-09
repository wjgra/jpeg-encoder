#ifndef _JPEG_BLOCK_GRID_HPP_
#define _JPEG_BLOCK_GRID_HPP_

#include <cstdint>
#include <array>
#include <iterator>

#include "..\inc\bitmap_image.hpp"

namespace jpeg{
class BlockGrid{
    public:
        BlockGrid(BitmapImageRGB const& input);
        uint16_t const static blockSize = 8; // Issue: include support for downsampling (e.g. via 16x16 blocks)

        struct Block{
            std::array<BitmapImageRGB::PixelData, blockSize * blockSize> data;
        };

        // Input iterator for accessing image blocks
        struct BlockIterator{
            using difference_type = std::ptrdiff_t;
            using value_type = Block;
            using underlying_type = BitmapImageRGB::PixelData;
            using underlying_pointer = underlying_type const *;
            using iterator_category = std::input_iterator_tag;
        private:
            underlying_pointer ptr; // Points to first (i.e. upper-left) pixel in block
            uint16_t blockRowPos, blockColPos; // Position in block-rows and block-columns
            uint16_t gridWidth, gridHeight;
        public:
            explicit BlockIterator() = default;
            BlockIterator(underlying_pointer p, uint16_t w, uint16_t h);
            value_type operator*() const;
            BlockIterator& operator++();
            BlockIterator operator++(int);
            friend bool operator==(BlockIterator const& a, BlockIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(BlockIterator const& a, BlockIterator const& b){return a.ptr != b.ptr;};
        private:
            bool isLastCol() const;
            bool isLastRow() const;
        };
        static_assert(std::input_iterator<BlockIterator>);
        
    public:
        BlockIterator begin();
        BlockIterator end();
    private:
        BitmapImageRGB const& imageData;
    };
};
#endif