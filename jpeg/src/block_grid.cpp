#include "block_grid.hpp"

jpeg::InputBlockGrid::InputBlockGrid(BitmapImageRGB const& input) : m_imageData{input}{};

jpeg::InputBlockGrid::BlockIterator::BlockIterator(underlying_pointer p, uint16_t w, uint16_t h) : 
    m_ptr{p}, m_blockRowPos{0}, m_blockColPos{0}, m_gridWidth{w}, m_gridHeight{h}{}

jpeg::InputBlockGrid::BlockIterator::value_type jpeg::InputBlockGrid::BlockIterator::operator*() const{
    Block output{};
    uint8_t const bottomRemainder = m_gridHeight % blockSize;
    uint8_t const rowsToOutput = isLastRow() ? (bottomRemainder == 0 ? blockSize : bottomRemainder): blockSize;
    uint8_t const rightRemainder = m_gridWidth % blockSize;
    uint8_t const colsToOutput = isLastCol() ? (rightRemainder == 0 ? blockSize : rightRemainder): blockSize;


    for (int row = 0 ; row < rowsToOutput ; ++row){
        for (int col = 0 ; col < colsToOutput ; ++col){
            output.m_blockPixelData[row * blockSize + col] = m_ptr[row * m_gridWidth + col];
        }
        for (int col = colsToOutput - 1 ; col < blockSize ; ++col){
            output.m_blockPixelData[row * blockSize + col] = m_ptr[row * m_gridWidth + colsToOutput - 1];
        }
    }   
    for (int row = rowsToOutput ; row < blockSize ; ++row){
        std::copy(output.m_blockPixelData.begin() + (rowsToOutput - 1) * blockSize, output.m_blockPixelData.begin() + rowsToOutput * blockSize, output.m_blockPixelData.begin() + row * blockSize);
    }
    return output;
}

jpeg::InputBlockGrid::BlockIterator& jpeg::InputBlockGrid::BlockIterator::operator++(){
    if (!isLastCol()){
        // Advance to next block in current block-row
        m_ptr += blockSize;
        ++m_blockColPos;
    }
    else{
        // Advance to start of next block-row 
        uint8_t offsetToNextDataRow = m_gridWidth % blockSize;
        if (offsetToNextDataRow == 0){
            offsetToNextDataRow = blockSize;
        }
        m_ptr += offsetToNextDataRow;
        if (!isLastRow()){
            m_ptr += (blockSize - 1) * m_gridWidth;
        }
        else{
            uint8_t remainingDataRows = (m_gridHeight % blockSize);
            if (remainingDataRows == 0){
                remainingDataRows = blockSize;
            }
            uint32_t offsetToNextBlockRow = (remainingDataRows - 1) * m_gridWidth;
            m_ptr += offsetToNextBlockRow;  
        }      
        m_blockColPos = 0;
        ++m_blockRowPos;
        }
    return *this;
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::BlockIterator::operator++(int){
    auto temp = *this; ++(*this); return temp;
}

bool jpeg::InputBlockGrid::BlockIterator::isLastCol() const{
    return ((m_blockColPos + 1) == (m_gridWidth / blockSize + ((m_gridWidth % blockSize) != 0)));
}

bool jpeg::InputBlockGrid::BlockIterator::isLastRow() const{
    return ((m_blockRowPos + 1) == (m_gridHeight / blockSize + ((m_gridHeight % blockSize) != 0)));
}

jpeg::InputBlockGrid::BlockIterator::underlying_pointer jpeg::InputBlockGrid::BlockIterator::getDataPtr() const{
    return m_ptr;
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::begin() const{
    return BlockIterator(m_imageData.m_imageData.data(), m_imageData.m_width, m_imageData.height);
}

jpeg::InputBlockGrid::BlockIterator jpeg::InputBlockGrid::end() const{
    return BlockIterator(m_imageData.m_imageData.data() + m_imageData.m_width * m_imageData.height, m_imageData.m_width, m_imageData.height);
}

jpeg::OutputBlockGrid::OutputBlockGrid(uint16_t width, uint16_t height) : 
    m_output{width, height},  m_blockGrid{m_output}, m_currentBlock{m_blockGrid.begin()}, m_gridWidth{width}, m_gridHeight{height}{}

void jpeg::OutputBlockGrid::processNextBlock(BlockGrid::Block const& inputBlock){
    uint8_t const bottomRemainder = m_gridHeight % blockSize;
    uint8_t const rowsToCopy = m_currentBlock.isLastRow() ? (bottomRemainder == 0 ? blockSize : bottomRemainder): blockSize;
    uint8_t const rightRemainder = m_gridWidth % blockSize;
    uint8_t const colsToCopy = m_currentBlock.isLastCol() ? (rightRemainder == 0 ? blockSize : rightRemainder): blockSize;

    for (size_t i = 0 ; i < rowsToCopy ; ++i){
                std::copy(inputBlock.m_blockPixelData.data() + i * blockSize ,
                          inputBlock.m_blockPixelData.data() + i * blockSize + colsToCopy, 
                          getBlockPtr() + i * m_gridWidth);
    }
    ++m_currentBlock;
}

jpeg::BitmapImageRGB jpeg::OutputBlockGrid::getBitmapRGB() const{
    if (m_currentBlock != m_blockGrid.end()){
        std::cerr << "Warning: not all JPEG blocks processed. Decoded Bitmap may be incomplete!\n";
    }
    return m_output;
}

bool jpeg::OutputBlockGrid::atEnd() const{
    return m_currentBlock == m_blockGrid.end();
}

jpeg::BitmapImageRGB::PixelData* jpeg::OutputBlockGrid::getBlockPtr(){
    return m_output.m_imageData.data() + (m_currentBlock.getDataPtr() - m_blockGrid.begin().getDataPtr());
}