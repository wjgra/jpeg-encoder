#ifndef _JPEG_BITSTREAM_HPP_
#define _JPEG_BITSTREAM_HPP_

#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>

namespace jpeg{

    /* Read progress for a BitStream object */
    struct BitStreamReadProgress{
        size_t currentByte;
        size_t currentBit;
        BitStreamReadProgress();
        void reset();
        void advanceBits(size_t numBits);
        void advanceBit();
        void advanceIntoAlignment();
    };

    /* A stream to which up to 16 bits can be pushed at a time.
       A vector of bytes representing the stream is retrievable as output. */
    class BitStream{
    public:
        BitStream();
        void clearStream();
        size_t getSize();
        void pushBitsu8(uint8_t data, size_t numberOfBitsToPush);
        void pushBitsu16(uint16_t data, size_t numberOfBitsToPush);
        uint8_t readByte(size_t byte) const;
        void pushByte(uint8_t data);
        void pushWord(uint16_t data);
        void pushIntoAlignment();
        bool readNextBit(BitStreamReadProgress& progress) const;
        uint8_t readNextAlignedByte(BitStreamReadProgress& progress) const;
        uint16_t readNextAlignedWord(BitStreamReadProgress& progress) const;
        uint8_t const* getDataPtr() const;
        void stuffBytes(size_t from);
        void removeStuffedBytes(BitStreamReadProgress const& progress);
    private:
        uint8_t m_buffer;
        size_t m_bitsInBuffer;
        std::vector<uint8_t> m_stream;
    };
}

#endif