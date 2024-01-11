#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include "..\inc\quantiser.hpp"
#include "..\inc\bitstream.hpp"
// temp
#include "..\inc\block_grid.hpp"


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
        EntropyChannelOutput encode(QuantisedChannelOutput const& input) const;
    private:
        QuantisedChannelOutput mapToZigZag(QuantisedChannelOutput const& input) const;
        RunLengthEncodedChannelOutput applyRunLengthEncoding(QuantisedChannelOutput const& input) const;
    protected:
        virtual EntropyChannelOutput applyEncoding(RunLengthEncodedChannelOutput const& input) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    protected:
        EntropyChannelOutput applyEncoding(RunLengthEncodedChannelOutput const& input) const override;
    };

}

#endif