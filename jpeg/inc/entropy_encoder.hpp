#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include <cstdint>
#include <vector>
#include <array>
#include <bit>
#include <unordered_map>
#include <map>
#include <ranges>
#include <span>
#include <algorithm>
#include <numeric>

#include "quantiser.hpp"
#include "bitstream.hpp"
#include "block_grid.hpp"
#include "markers.hpp"

namespace jpeg{
    struct RunLengthEncodedBlockChannelData{
        int16_t m_dcDifference;
        struct RunLengthEncodedACCoefficient{
            size_t m_runLength;
            int16_t m_value;
            bool operator==(RunLengthEncodedACCoefficient const&) const = default;
        };
        std::vector<RunLengthEncodedACCoefficient> m_acCoefficients;
    };

    class EntropyEncoder{
    public:
        EntropyEncoder() = default;
        EntropyEncoder(EntropyEncoder const&) = delete;
        EntropyEncoder(EntropyEncoder const&&) = delete;
        EntropyEncoder& operator=(EntropyEncoder const&) = delete;
        EntropyEncoder& operator=(EntropyEncoder const&&) = delete;
        virtual ~EntropyEncoder() = default;
    public:
        void encode(QuantisedBlockChannelData const& input, int16_t& lastDCValue, BitStream& outputStream, bool isLuminanceComponent) const;
        QuantisedBlockChannelData decode(BitStream const& inputStream, BitStreamReadProgress& readProgress, int16_t& lastDCValue, bool isLuminanceComponent) const;
        virtual void encodeHeaderEntropyTables(BitStream& outputStream) const = 0;
        /* Issue: include decoding for non-default tables */
    private:
        QuantisedBlockChannelData mapFromGridToZigZag(QuantisedBlockChannelData const& input) const;
        QuantisedBlockChannelData mapFromZigZagToGrid(QuantisedBlockChannelData const& input) const;
        RunLengthEncodedBlockChannelData applyRunLengthEncoding(QuantisedBlockChannelData const& input, int16_t& lastDCValue) const;
        QuantisedBlockChannelData removeRunLengthEncoding(RunLengthEncodedBlockChannelData const& input, int16_t& lastDCValue) const;
    protected:
        virtual void applyFinalEncoding(RunLengthEncodedBlockChannelData const& input, BitStream& outputStream, bool isLuminanceComponent) const = 0;
        virtual RunLengthEncodedBlockChannelData removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    public:
        HuffmanEncoder();
        void encodeHeaderEntropyTables(BitStream& outputStream) const override;
    protected:
        void applyFinalEncoding(RunLengthEncodedBlockChannelData const& input, BitStream& outputStream, bool isLuminanceComponent) const override;
        RunLengthEncodedBlockChannelData removeFinalEncoding(BitStream const& inputStream, BitStreamReadProgress& readProgress, bool isLuminanceComponent) const override;
    private:
        struct HuffmanTable{
            struct HuffmanCode{
                size_t m_codeLength;
                uint16_t m_codeWord;
                bool operator==(HuffmanCode const&) const = default;
            };
            std::array<HuffmanCode const, 12> m_dcTable;
            std::unordered_map<uint16_t, size_t> m_dcLookup;
            std::array<std::array<HuffmanCode const, 10>, 16> m_acTable;
            HuffmanCode m_acEndOfBlock, m_acZeroRunLength;
            struct HuffIndexAC{
            size_t m_RRRR, m_SSSS;
            };
            std::unordered_map<uint16_t, HuffIndexAC> m_acLookup;
        };
        HuffmanTable m_luminanceHuffTable;
        HuffmanTable m_chrominanceHuffTable;
        void pushHuffmanCodedDCDifferenceToStream(int16_t dcDifference, BitStream& outputStream, HuffmanTable const& huffTable) const;
        void pushHuffmanCodedACCoefficientToStream(RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient acCoeff, BitStream& outputStream, HuffmanTable const& huffTable) const;
        uint16_t static appendBit(uint16_t input, bool bit){return (input << 1) | uint16_t(bit);}
        int16_t extractDCDifferenceFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const;
        RunLengthEncodedBlockChannelData::RunLengthEncodedACCoefficient extractACCoefficientFromStream(BitStream const& inputStream, BitStreamReadProgress& readProgress, HuffmanTable const& huffTable) const;
    };  
    /* To be implemented! */
    /* class ArithmeticEncoder : public EntropyEncoder{
    }; */
}

#endif