#ifndef _JPEG_BITSTREAM_HPP_
#define _JPEG_BITSTREAM_HPP_

#include <vector>
#include <cstdint>

namespace jpeg{

    /* A stream to which up to 8 bits can be pushed at a time.
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
        void pushBits(uint8_t data, uint8_t numberOfBitsToPush){
            int offset = 8 - bitsInBuffer - numberOfBitsToPush;
            if (offset >= 0){
                // Bits fit in buffer
                buffer |= uint8_t(data << offset);
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
        std::vector<uint8_t> getStream() const{
            auto temp = stream;
            temp.push_back(buffer);
            return temp;
        }
    private:
        uint8_t buffer;
        uint8_t bitsInBuffer;
        std::vector<uint8_t> stream;
    };
}

#endif