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
        
        struct Block{
            std::array<BitmapImageRGB::PixelData, blockSize * blockSize> data;
        };
        // Input iterator for accessing image blocks
        struct NewBlockIterator{
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
            explicit NewBlockIterator() = default;
            NewBlockIterator(underlying_pointer p, uint16_t w, uint16_t h) : ptr{p}, gridWidth{w}, gridHeight{h}, 
                blockRowPos{0}, blockColPos{0}{};
            value_type operator*() const{
                Block output{};
                // bool isLastCol = ((blockColPos + 1) == blockSize);
                uint8_t paddingRight = isLastCol() ? blockSize - (gridWidth % blockSize) : 0;
                // bool isLastRow = ((blockRowPos + 1 ) == blockSize);
                uint8_t paddingBottom = isLastRow() ? blockSize - (gridHeight % blockSize) : 0;
                // Populate output block
                for (int row = 0; row < blockSize - paddingBottom ; ++row){
                    int col = 0;
                    for (; col < blockSize - paddingRight ; ++col){
                        output.data[row * blockSize + col] = ptr[row * gridWidth + col];
                    }
                    // Pad excess columns with copies of last entry in row
                    for (; col < blockSize ; ++col){
                        output.data[row * blockSize + col] = ptr[row * gridWidth + (blockSize - paddingRight - 1)];
                    }
                }
                // Pad excess rows in block with copies of last row
                for (int row = blockSize - paddingBottom; row < blockSize ; ++row){
                    std::copy(output.data.begin() + (blockSize - paddingBottom - 1) * blockSize, output.data.begin() + (blockSize - paddingBottom) * blockSize, output.data.begin() + row * blockSize);
                }
                return output;
            };
            NewBlockIterator& operator++(){
                if (!isLastCol()){
                    // Advance to next block in current block-row
                    ptr += blockSize;
                    ++blockColPos;
                }
                else{
                    // Advance to start of next block-row 
                    uint8_t offsetToNextDataRow = gridWidth % blockSize;
                    if (offsetToNextDataRow == 0){
                        offsetToNextDataRow = blockSize;
                    }
                    ptr += offsetToNextDataRow;
                    if (!isLastRow()){
                        ptr += (blockSize - 1) * gridWidth;
                    }
                    else{
                        uint8_t remainingDataRows = (gridHeight % blockSize);
                        if (remainingDataRows == 0){
                            remainingDataRows = blockSize;
                        }
                        uint8_t offsetToNextBlockRow = (remainingDataRows - 1) * gridWidth;
                        ptr += offsetToNextBlockRow;  
                    }      
                    blockColPos = 0;
                    ++blockRowPos;
                }
                return *this;
            };
            NewBlockIterator operator++(int){
                auto temp = *this; ++(*this); return temp;
            };

            friend bool operator==(NewBlockIterator const& a, NewBlockIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(NewBlockIterator const& a, NewBlockIterator const& b){return a.ptr != b.ptr;};
        private:
            bool isLastCol() const{
                return ((blockColPos + 1) == (gridWidth / blockSize + ((gridWidth % blockSize) != 0)));
            };
            bool isLastRow() const{
                return ((blockRowPos + 1) == (gridHeight / blockSize + ((gridHeight % blockSize) != 0)));
            };
        };
        static_assert(std::input_iterator<NewBlockIterator>);

        public:
        NewBlockIterator begin(){
            return NewBlockIterator(imageData.data.data(), imageData.width, imageData.height);
        };
        NewBlockIterator end(){
            return NewBlockIterator(imageData.data.data() + imageData.width * imageData.height,
            
            //+ /* Size of block-row */(blockSize * imageData.width) 
              //                                            * /* Number of block-rows */(imageData.height / blockSize + ((imageData.height % blockSize) != 0))
                //                                          + /* Number of block-cols */(imageData.width / blockSize + ((imageData.width % blockSize) != 0)), 
                                    imageData.width, imageData.height);
        };

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


        // Input iterator for accessing the blocks of pixel data for a given 'block-row'
        struct BlockIterator{
            // really, value type should be 8x8 array here...
            using difference_type = std::ptrdiff_t;
            using value_type = Block;
            using pointer_type = value_type *;
            using reference_type = value_type const&;
            using iterator_category = std::input_iterator_tag;
        private:
            BitmapImageRGB::PixelData const* ptr; // Points to first (i.e. upper-left) pixel in block
            uint16_t rowPos;
            uint8_t paddingBottom, paddingEndOfRow;
            uint16_t gridWidth;
        public:
            explicit BlockIterator() = default;
            BlockIterator(RowIterator rowIt, uint16_t posInRow = 0, uint8_t padDown = 0, uint8_t padRight = 0) : ptr{rowIt.operator->()}, rowPos{posInRow}, paddingBottom{padDown}, paddingEndOfRow{padRight}, gridWidth{rowIt.getGridWidth()}{
                ptr += rowPos * blockSize;
            };
            
            value_type operator*() const {
                Block output{};
                bool isLastCol = false;
                uint8_t paddingRight = isLastCol ? paddingEndOfRow : 0;
                for (int row = 0; row < blockSize - paddingBottom ; ++row){
                    int col = 0;
                    for (; col < blockSize - paddingRight ; ++col){
                        output.data[row * blockSize + col] = ptr[row * gridWidth + col];
                    }
                    // Pad excess columns with copies of last entry in row
                    for (; col < blockSize ; ++col){
                        output.data[row * blockSize + col] = ptr[row * gridWidth + (blockSize - paddingRight - 1)];
                    }
                }
                // Pad excess rows in block with copies of last row
                for (int row = blockSize - paddingBottom; row < blockSize ; ++row){
                    std::copy(output.data.begin() + row * blockSize, output.data.begin() + (row + 1) * blockSize, output.data.begin() + (row + 1) * blockSize);
                }
                return output;
                
                
            };// *ptr ;}; // implement padding logic

            BlockIterator& operator++() {ptr += blockSize; ++rowPos; return *this;};
            BlockIterator operator++(int) {auto temp = *this; ++(*this); return temp;};

            // pointer_type operator->() {return ptr;};
            friend bool operator==(BlockIterator const& a, BlockIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(BlockIterator const& a, BlockIterator const& b){return a.ptr != b.ptr;};
            
        };
        static_assert(std::input_iterator<BlockIterator>);
    public:
        //RowIterator begin(){return RowIterator(imageData.data.data(), imageData.width, imageData.height);};
        //RowIterator end(){return RowIterator(imageData.data.data() + /* Size of block-row */(blockSize * imageData.width) 
         //                                                          * /* Number of block-rows */(imageData.height / blockSize + ((imageData.height % blockSize) != 0)), 
         //                                    imageData.width, imageData.height);};
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