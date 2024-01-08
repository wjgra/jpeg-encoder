#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>
#include <vector>
#include <iterator>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"
#include "..\inc\colour_mapping.hpp"

namespace jpeg{

    class BlockGrid{
    public:
        BlockGrid(BitmapImageRGB const& input) : imageData{input}{};
        uint16_t const static blockSize = 8; // Issue: include support for downsampling (e.g. via 16x16 blocks)
        
        // Input iterator for accessing first element of a 'block-row'
        struct RowIterator{
            using difference_type = std::ptrdiff_t;
            using value_type = const BitmapImageRGB::PixelData;
            using pointer_type = value_type *;
            using reference_type = value_type &;
            using iterator_category = std::input_iterator_tag;
        private:
            pointer_type ptr;
            uint16_t gridWidth, gridHeight;
        public:
            explicit RowIterator() = default;
            RowIterator(pointer_type p, uint16_t w, uint16_t h) : ptr{p}, gridWidth{w}, gridHeight{h}{};
            reference_type operator*() const {return *ptr;};
            RowIterator& operator++() {ptr += blockSize * gridWidth; return *this;};
            RowIterator operator++(int) {auto temp = *this; ++(*this); return temp;};

            pointer_type operator->() const {return ptr;};
            friend bool operator==(RowIterator const& a, RowIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(RowIterator const& a, RowIterator const& b){return a.ptr != b.ptr;};
            uint16_t getGridWidth() const {return gridWidth;};
            uint16_t getGridHeight() const {return gridHeight;};
            bool isLastRow() const {return false;};
        };
        static_assert(std::input_iterator<RowIterator>);

        struct Block{
            std::array<BitmapImageRGB::PixelData, blockSize * blockSize> data;
        };

        // Input iterator for accessing the blocks of pixel data for a given 'block-row'
        struct BlockIterator{
            // really, value type should be 8x8 array here...
            using difference_type = std::ptrdiff_t;
            using value_type = const Block;
            using pointer_type = value_type *;
            using reference_type = value_type &;
            using iterator_category = std::input_iterator_tag;
        private:
            BitmapImageRGB::PixelData const* ptr; // Points to first (i.e. upper-left) pixel in block
            uint16_t rowPos;
            uint8_t paddingBottom, paddingEndOfRow;
        public:
            explicit BlockIterator() = default;
            BlockIterator(RowIterator rowIt, uint16_t posInRow = 0, uint8_t padDown = 0, uint8_t padRight = 0) : ptr{rowIt.operator->()}, rowPos{posInRow}, paddingBottom{padDown}, paddingEndOfRow{padRight}{
/*                 if (rowPos >= rowIt.getGridWidth()){
                    throw std::runtime_error("BlockIterator: invalid row position");
                } */
                ptr += rowPos;
            };
            
            reference_type operator*() const {
                
                
                return value_type();
                
                
            };// *ptr ;}; // implement padding logic

            BlockIterator& operator++() {++ptr; ++rowPos; return *this;};
            BlockIterator operator++(int) {auto temp = *this; ++(*this); return temp;};

            // pointer_type operator->() {return ptr;};
            friend bool operator==(BlockIterator const& a, BlockIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(BlockIterator const& a, BlockIterator const& b){return a.ptr != b.ptr;};
            
        };
        static_assert(std::input_iterator<BlockIterator>);
    public:
        RowIterator begin(){return RowIterator(imageData.data.data(), imageData.width, imageData.height);};
        RowIterator end(){return RowIterator(imageData.data.data() + /* Size of block-row */(blockSize * imageData.width) 
                                                                   * /* Number of block-rows */(imageData.height / blockSize + ((imageData.height % blockSize) != 0)), 
                                             imageData.width, imageData.height);};
        // Iterate blocks over a row
        BlockIterator beginRow(RowIterator const& rowIt){
            uint8_t paddingBelow = rowIt.isLastRow() ? (blockSize - (imageData.height % blockSize)) % blockSize : 0;
            uint8_t paddingEndOfRow = (blockSize - (imageData.width % blockSize)) % blockSize;
            return BlockIterator(rowIt, 0, paddingBelow, paddingEndOfRow);
        };
        BlockIterator endRow(RowIterator const& rowIt){
            uint16_t blocksInRow = imageData.width / blockSize + ((imageData.width % blockSize) != 0);
            return BlockIterator(rowIt, blocksInRow);};
    private:
        BitmapImageRGB const& imageData;
    };

    class DiscreteCosineTransformer{
    public:
    };
    class Quantiser{
    public:
    };
    class EntropyEncoder{
    public:
    };

    class Encoder{
    protected:
        Encoder(BitmapImageRGB const& inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransform,
                Quantiser const& quantisation,
                EntropyEncoder const& entropyEncoder);
    public:
        bool saveJPEGToFile(std::string const& savePath);
        JPEGImage getJPEGImageData();
    private:
        JPEGImage jpegImageData;
        // references to encoder components
        /* ColourMapper const& colourMapper;
        BlockGenerator const& blockGenerator;
        DiscreteCosineTransformer const& discreteCosineTransform;
        Quantiser const& quantisation;
        EntropyEncoder const& entropyEncoder; */
    };

    class JPEGEncoder : public Encoder{
    public:
        JPEGEncoder(BitmapImageRGB const& inputImage);
    };
};

#endif