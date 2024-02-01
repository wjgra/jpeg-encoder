#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include <cstdint>
#include <vector>
#include <array>
#include <bit>
#include <unordered_map>
#include <ranges>
#include <span>
#include <algorithm>

#include "..\inc\quantiser.hpp"
#include "..\inc\bitstream.hpp"
#include "..\inc\block_grid.hpp"
#include "..\inc\markers.hpp"

namespace jpeg{

    
    struct RunLengthEncodedChannelOutput{ // Issue: consider renaming all 'output' types to 'data', or just nothing
        int16_t dcDifference;
        struct RunLengthEncodedACCoefficient{
            size_t runLength;
            int16_t value;
            bool operator==(RunLengthEncodedACCoefficient const& other) const = default;
        };
        std::vector<RunLengthEncodedACCoefficient> acCoefficients;
    };

    class EntropyEncoder{
    public:
        void encode(QuantisedChannelOutput const& input, int16_t& lastDCValue, BitStream& outputStream, bool isLuminanceComponent) const;
        QuantisedChannelOutput decode(BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue, bool isLuminanceComponent) const;
        virtual void encodeHeaderEntropyTables(BitStream& outputStream) const = 0;
        /* Issue: include decoding for non-default tables */
    private:
        QuantisedChannelOutput mapFromGridToZigZag(QuantisedChannelOutput const& input) const;
        QuantisedChannelOutput mapFromZigZagToGrid(QuantisedChannelOutput const& input) const;
        RunLengthEncodedChannelOutput applyRunLengthEncoding(QuantisedChannelOutput const& input, int16_t& lastDCValue) const;
        QuantisedChannelOutput removeRunLengthEncoding(RunLengthEncodedChannelOutput const& input, int16_t& lastDCValue) const;
    protected:
        virtual void applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream, bool isLuminanceComponent) const = 0;
        virtual RunLengthEncodedChannelOutput removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    public:
        HuffmanEncoder();
        void encodeHeaderEntropyTables(BitStream& outputStream) const override;
    protected:
        void applyFinalEncoding(RunLengthEncodedChannelOutput const& input, BitStream& outputStream, bool isLuminanceComponent) const override;
        RunLengthEncodedChannelOutput removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const override;
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
        HuffmanTable chrominanceHuffTable;
        void pushHuffmanCodedDCDifferenceToStream(int16_t dcDifference, BitStream& outputStream, HuffmanTable const& huffTable) const;
        void pushHuffmanCodedACCoefficientToStream(RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient acCoeff, BitStream& outputStream, HuffmanTable const& huffTable) const;
        uint16_t static appendBit(uint16_t input, bool bit){return (input << 1) | uint16_t(bit);}
        int16_t extractDCDifferenceFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const;
        RunLengthEncodedChannelOutput::RunLengthEncodedACCoefficient extractACCoefficientFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const;
    
    };  
    /* To be implemented! */
    /* class ArithmeticEncoder : public EntropyEncoder{
    }; */
}

#endif