#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include "..\inc\quantiser.hpp"
#include "..\inc\bitstream.hpp"
// temp
#include "..\inc\block_grid.hpp"

#include <cstdint>
#include <vector>
#include <array>
#include <bit>
#include <unordered_map>
#include <ranges>
#include <span>

namespace jpeg{

    
    struct RunLengthEncodedChannelOutput{ // Issue: consider renaming all 'output' types to 'data', or just nothing
        int16_t dcDifference;
        struct RunLengthEncodedACCoefficients{
            uint8_t runLength;
            int16_t value;
        }; // these values are processed into RRRRSSSS in the huffman stage
        std::vector<RunLengthEncodedACCoefficients> acCoefficients;
    };

    class EntropyEncoder{
    public:
        void encode(QuantisedChannelOutput const& input, int16_t& lastDCValue, BitStream& outputStream) const;
        QuantisedChannelOutput decode(BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue) const;
    private:
        QuantisedChannelOutput mapFromGridToZigZag(QuantisedChannelOutput const& input) const;
        QuantisedChannelOutput mapFromZigZagToGrid(QuantisedChannelOutput const& input) const;
        RunLengthEncodedChannelOutput applyRunLengthEncoding(QuantisedChannelOutput const& input, int16_t& lastDCValue) const;
        QuantisedChannelOutput removeRunLengthEncoding(RunLengthEncodedChannelOutput const& input, int16_t& lastDCValue) const;
    protected:
        virtual void applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream) const = 0;
        virtual RunLengthEncodedChannelOutput removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    public:
        HuffmanEncoder();
    protected:
        void applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream) const override;
        RunLengthEncodedChannelOutput removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress) const override;
    private:
        struct HuffmanTable{
            struct HuffmanCode{
                size_t codeLength;
                uint16_t codeWord;
                bool operator==(HuffmanCode const& other) const {
                    return codeLength == other.codeLength && codeWord == other.codeWord;
                }
            };
            std::array<HuffmanCode const, 12> dcTable;
            std::unordered_map<uint16_t, size_t> dcLookup;
            std::array<std::array<HuffmanCode const, 10>, 16> acTable;
            HuffmanCode acEndOfBlock, acZeroRunLength;
            struct HuffIndexAC{
            size_t RRRR, SSSS;
            };
            std::unordered_map<uint16_t, HuffIndexAC> acLookup;
        };
        HuffmanTable luminanceHuffTable;
        // HuffmanTable chromaticityHuffTable;
    };

    /* Could implement arithmetic coding by inheriting from EntropyEncoder and implementing apply/remove FinalEncoding */
}

#endif