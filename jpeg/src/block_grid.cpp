#include "..\inc\block_grid.hpp"

jpeg::BlockGrid::BlockGrid(BitmapImageRGB const& input) : imageData{input}{};

jpeg::BlockGrid::BlockIterator::BlockIterator(underlying_pointer p, uint16_t w, uint16_t h) : 
    ptr{p}, blockRowPos{0}, blockColPos{0}, gridWidth{w}, gridHeight{h}{}

jpeg::BlockGrid::BlockIterator::value_type jpeg::BlockGrid::BlockIterator::operator*() const{
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

jpeg::BlockGrid::BlockIterator& jpeg::BlockGrid::BlockIterator::operator++(){
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

jpeg::BlockGrid::BlockIterator jpeg::BlockGrid::BlockIterator::operator++(int){
    auto temp = *this; ++(*this); return temp;
}

bool jpeg::BlockGrid::BlockIterator::isLastCol() const{
    return ((blockColPos + 1) == (gridWidth / blockSize + ((gridWidth % blockSize) != 0)));
}

bool jpeg::BlockGrid::BlockIterator::isLastRow() const{
    return ((blockRowPos + 1) == (gridHeight / blockSize + ((gridHeight % blockSize) != 0)));
}

jpeg::BlockGrid::BlockIterator jpeg::BlockGrid::begin(){
    return BlockIterator(imageData.data.data(), imageData.width, imageData.height);
}

jpeg::BlockGrid::BlockIterator jpeg::BlockGrid::end(){
    return BlockIterator(imageData.data.data() + imageData.width * imageData.height, imageData.width, imageData.height);
}