#include "bit_stream.hpp"

jpeg::BitStreamReadProgress::BitStreamReadProgress(){
    reset();
}

void jpeg::BitStreamReadProgress::reset(){
    currentByte = 0;
    currentBit = 0;
}

void jpeg::BitStreamReadProgress::advanceBits(size_t numBits){
    currentBit += numBits;
    while (currentBit >= 8){
        ++currentByte;
        currentBit -= 8;
    }
}

void jpeg::BitStreamReadProgress::advanceBit(){
    advanceBits(1);
}

void jpeg::BitStreamReadProgress::advanceIntoAlignment(){
    if (currentBit > 0){
        currentBit = 0;
        ++currentByte;
    }
}

jpeg::BitStream::BitStream(){
    clearStream();
}
void jpeg::BitStream::clearStream(){
    m_buffer = 0;
    m_bitsInBuffer = 0;
    m_stream.clear();
}

size_t jpeg::BitStream::getSize(){
    return m_stream.size() + (m_bitsInBuffer > 0);
}

void jpeg::BitStream::pushBitsu8(uint8_t data, size_t numberOfBitsToPush){
    assert(numberOfBitsToPush <= 8);
    // Remove unneeded leading bits
    data &= uint8_t(0xFF) >> (8 - numberOfBitsToPush);
    int offset = 8 - m_bitsInBuffer - numberOfBitsToPush;
    if (offset >= 0){
        // Bits fit in buffer
        m_buffer |= uint8_t((data) << offset);
        m_bitsInBuffer += numberOfBitsToPush;
        if (offset == 0){
            // Push buffer to stream
            m_stream.push_back(m_buffer);
            m_buffer = 0;
            m_bitsInBuffer = 0;
        }
    }
    else{
        // Bits do not fit in buffer
        m_buffer |= uint8_t(data >> -offset);
        m_stream.push_back(m_buffer);
        m_buffer = uint8_t(data << (8 + offset));
        m_bitsInBuffer = -offset;
    }
}

void jpeg::BitStream::pushBitsu16(uint16_t data, size_t numberOfBitsToPush){
    assert(numberOfBitsToPush <= 16);
    if (numberOfBitsToPush <= 8){
        uint8_t const u8Data = data & uint8_t(0x00FF);
        pushBitsu8(u8Data, numberOfBitsToPush);
    }
    else{
        uint8_t const u8LeftData = (data >> 8) & uint8_t(0x00FF);
        pushBitsu8(u8LeftData, numberOfBitsToPush - 8);
        uint8_t const u8RightData = data & uint8_t(0x00FF);
        pushBitsu8(u8RightData, 8);
    }
}

uint8_t jpeg::BitStream::readByte(size_t byte) const{
    if (byte == m_stream.size()){
        return m_buffer;
    }
    else{
        return m_stream[byte];
    }
    
}

void jpeg::BitStream::pushByte(uint8_t data){
    pushBitsu8(data, 8);
}

void jpeg::BitStream::pushWord(uint16_t data){
    pushBitsu16(data, 16);
}

void jpeg::BitStream::pushIntoAlignment(){
    if (m_bitsInBuffer > 0){
        pushBitsu8(0, 8 - m_bitsInBuffer);
    }
}

bool jpeg::BitStream::readNextBit(BitStreamReadProgress& progress) const{
    uint8_t currentByte = readByte(progress.currentByte);
    bool output = currentByte & (1u << (7 - progress.currentBit));
    progress.advanceBit();
    return output;
}

uint8_t jpeg::BitStream::readNextAlignedByte(BitStreamReadProgress& progress) const{
    progress.advanceIntoAlignment();
    return readByte(progress.currentByte++);
}

uint16_t jpeg::BitStream::readNextAlignedWord(BitStreamReadProgress& progress) const{
    uint8_t firstByte = readNextAlignedByte(progress);
    return (firstByte << 8) | readNextAlignedByte(progress);
}
uint8_t const* jpeg::BitStream::getDataPtr() const{
    return m_stream.data();
}

void jpeg::BitStream::stuffBytes(size_t from){
    for (auto byte = m_stream.begin() + from ; byte != m_stream.end() ; ++byte){
        if (*byte == 0xFF){
            ++byte;
            m_stream.insert(byte, 0x00);
        }
    }
}

void jpeg::BitStream::removeStuffedBytes(BitStreamReadProgress const& progress){
    for (auto byte = m_stream.begin() + progress.currentByte ; byte != m_stream.end() - 2 /* Ignore EOI marker */; ){              
        if (*byte == 0xFF && *(++byte) == 0x00){
            // Note that due to byte stuffing it is not possible for two 0xFF bytes to be consecutive
            m_stream.erase(byte);
        }
        else{
            ++byte;
        }

    }
}