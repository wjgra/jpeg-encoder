#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include "..\inc\quantiser.hpp"
#include "..\inc\bitstream.hpp"
// temp
#include "..\inc\block_grid.hpp"

#include <cstdint>
#include <vector>
#include <array>

namespace jpeg{

    
    struct RunLengthEncodedChannelOutput{ // Issue: consider renaming all 'output' types to 'data', or just nothing
        int16_t dcDifference;
        struct RunLengthEncodedACCoefficients{
            uint8_t runLength;
            int16_t value;
        }; // these values are processed into RRRRSSSS in the huffman stage
        std::vector<RunLengthEncodedACCoefficients> acCoefficients;
    };
    struct EntropyChannelOutput{
        /* BitStream dc;
        std::vector<uint8_t> data; */
        RunLengthEncodedChannelOutput temp;
    };

    class EntropyEncoder{
    public:
        EntropyChannelOutput encode(QuantisedChannelOutput const& input, int16_t& lastDCValue) const;/* non const if it keeps track of the coeff */
        QuantisedChannelOutput decode(EntropyChannelOutput const& input, int16_t& lastDCValue) const;
    private:
        QuantisedChannelOutput mapFromGridToZigZag(QuantisedChannelOutput const& input) const;
        QuantisedChannelOutput mapFromZigZagToGrid(QuantisedChannelOutput const& input) const;
        RunLengthEncodedChannelOutput applyRunLengthEncoding(QuantisedChannelOutput const& input, int16_t& lastDCValue) const;
        QuantisedChannelOutput removeRunLengthEncoding(RunLengthEncodedChannelOutput const& input, int16_t& lastDCValue) const;
    protected:
        virtual EntropyChannelOutput applyFinalEncoding(RunLengthEncodedChannelOutput const& input) const = 0;
        virtual RunLengthEncodedChannelOutput removeFinalEncoding(EntropyChannelOutput const& input) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    protected:
        EntropyChannelOutput applyFinalEncoding(RunLengthEncodedChannelOutput const& input) const override;
        RunLengthEncodedChannelOutput removeFinalEncoding(EntropyChannelOutput const& input) const override;
    };

}

#endif