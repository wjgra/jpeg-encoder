#include "..\inc\block_grid.hpp"

jpeg::InputBlockGrid::InputBlockGrid(BitmapImageRGB const& input) : imageData{input}{};

jpeg::InputBlockGrid::BlockIterator::BlockIterator(underlying_pointer p, uint16_t w, uint16_t h) : 
    ptr{p}, blockRowPos{0}, blockColPos{0}, gridWidth{w}, gridHeight{h}{}

jpeg::InputBlockGrid::BlockIterator::value_type jpeg::InputBlockGrid::BlockIterator::operator*() const{
    Block output{};
    uint8_t paddingRight = isLastCol() ? blockSize - (gridWidth % blockSize) : 0;
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
}

jpeg::InputBlockGrid::BlockIterator& jpeg::InputBlockGrid::BlockIterator::operator++(){
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
            uint32_t offsetToNextBlockRow = (remainingDataRows - 1) * gridWidth;
            ptr += offsetToNextBlockRow;  
        }      
        blockColPos = 0;
        ++blockRowPos;
        }
    return *this;
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::BlockIterator::operator++(int){
    auto temp = *this; ++(*this); return temp;
}

bool jpeg::InputBlockGrid::BlockIterator::isLastCol() const{
    return ((blockColPos + 1) == (gridWidth / blockSize + ((gridWidth % blockSize) != 0)));
}

bool jpeg::InputBlockGrid::BlockIterator::isLastRow() const{
    return ((blockRowPos + 1) == (gridHeight / blockSize + ((gridHeight % blockSize) != 0)));
}

jpeg::InputBlockGrid::BlockIterator::underlying_pointer jpeg::InputBlockGrid::BlockIterator::getDataPtr() const{
    return ptr;
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::begin() const{
    return BlockIterator(imageData.data.data(), imageData.width, imageData.height);
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::end() const{
    return BlockIterator(imageData.data.data() + imageData.width * imageData.height, imageData.width, imageData.height);
}

jpeg::OutputBlockGrid::OutputBlockGrid(uint16_t width, uint16_t height) : 
    output{width, height},  blockGrid{output}, currentBlock{blockGrid.begin()}, w{width}, h{height}{}

void jpeg::OutputBlockGrid::processNextBlock(BlockGrid::Block const& inputBlock){
    uint8_t const bottomPadding = h % blockSize;
    uint8_t const rowsToCopy = currentBlock.isLastRow() ? (bottomPadding == 0 ? blockSize : bottomPadding): blockSize;
    uint8_t const rightPadding = w % blockSize;
    uint8_t const colsToCopy = currentBlock.isLastCol() ? (rightPadding == 0 ? blockSize : rightPadding): blockSize;

    for (size_t i = 0 ; i < rowsToCopy ; ++i){
                std::copy(inputBlock.data.data() + i * blockSize ,
                          inputBlock.data.data() + i * blockSize + colsToCopy, 
                          getBlockPtr() + i * w);
    }
    ++currentBlock;
}

jpeg::BitmapImageRGB jpeg::OutputBlockGrid::getBitmapRGB() const{
    if (currentBlock != blockGrid.end()){
        std::cerr << "Warning: not all JPEG blocks processed. Decoded Bitmap may be incomplete!\n";
    }
    return output;
}

jpeg::BitmapImageRGB::PixelData* jpeg::OutputBlockGrid::getBlockPtr(){
    return output.data.data() + (currentBlock.getDataPtr() - blockGrid.begin().getDataPtr());
}