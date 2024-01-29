#ifndef _JPEG_BITSTREAM_HPP_
#define _JPEG_BITSTREAM_HPP_

#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>

namespace jpeg{

    struct BitStreamReadProgress{ // consider including in BitStream
        size_t currentByte;
        size_t currentBit;
        BitStreamReadProgress(){
            reset();
        }
        void reset(){
            currentByte = 0;
            currentBit = 0;
        }
        void advanceBits(size_t numBits){
            currentBit += numBits;
            while (currentBit >= 8){
                ++currentByte;
                currentBit -= 8;
            }
        }
        void advanceBit(){
            advanceBits(1);
        }
    };



    /* A stream to which up to 16 bits can be pushed at a time.
       A vector of bytes representing the stream is retrievable as output. */
    class BitStream{
    public:
        BitStream(){
            clearStream();
        }
        void clearStream(){
            buffer = 0;
            bitsInBuffer = 0;
            stream.clear();
        }

        std::uintmax_t getSize(){
            return stream.size() + (bitsInBuffer > 0);
        }
        static bool getBit(uint8_t input, size_t pos)
        {
            return input & (1u << (7 - pos));
        }


        void pushBits(uint8_t data, size_t numberOfBitsToPush){
            // std::cout << "PB: " << int(data) << ", " << numberOfBitsToPush << "\n";
            assert(numberOfBitsToPush <= 8);
            // Remove unneeded leading bits
            data &= uint8_t(0xFF) >> (8 - numberOfBitsToPush);
            int offset = 8 - bitsInBuffer - numberOfBitsToPush;
            if (offset >= 0){
                // Bits fit in buffer
                buffer |= uint8_t((data) << offset);
                bitsInBuffer += numberOfBitsToPush;
                if (offset == 0){
                    // Push buffer to stream
                    stream.push_back(buffer);
                    buffer = 0;
                    bitsInBuffer = 0;
                }
            }
            else{
                // Bits do not fit in buffer
                buffer |= uint8_t(data >> -offset);
                stream.push_back(buffer);
                buffer = uint8_t(data << (8 + offset));
                bitsInBuffer = -offset;
            }
        }
        
        void pushBits(uint16_t data, size_t numberOfBitsToPush){
            assert(numberOfBitsToPush <= 16);
            if (numberOfBitsToPush <= 8){
                uint8_t const u8Data = data & uint8_t(0x00FF);
                pushBits(u8Data, numberOfBitsToPush);
            }
            else{
                uint8_t const u8LeftData = (data >> 8) & uint8_t(0x00FF);
                pushBits(u8LeftData, numberOfBitsToPush - 8);
                uint8_t const u8RightData = data & uint8_t(0x00FF);
                pushBits(u8RightData, 8);
            }
        }
        
        uint8_t readByte(size_t byte) const{
            if (byte == stream.size()){
                return buffer;
            }
            else{
                return stream[byte];
            }
            
        }

        bool readNextBit(BitStreamReadProgress& progress) const{
            uint8_t currentByte = readByte(progress.currentByte);
            bool output = getBit(currentByte, progress.currentBit);
            progress.advanceBit();
            return output;
        }

        /* bool readNextBits(BitStreamReadProgress& progress, size_t numberOfBits) const{
            uint8_t currentByte = readByte(progress.currentByte);
            bool output = getBit(currentByte, progress.currentBit);
            progress.advanceBit();
            return output;
        } */



       /*  void finishStream(){
            stream.push_back(buffer);
            bitsInBuffer = 0;
        } */

        std::vector<uint8_t> getStream() const{
            auto temp = stream;
            temp.push_back(buffer);
            return temp;
        }
    private:
        uint8_t buffer;
        size_t bitsInBuffer;
        std::vector<uint8_t> stream;
    };
}

#endif