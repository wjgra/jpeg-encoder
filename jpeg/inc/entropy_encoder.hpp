#ifndef _JPEG_ENTROPY_ENCODER_HPP_
#define _JPEG_ENTROPY_ENCODER_HPP_

#include "..\inc\quantiser.hpp"

#include <vector>

namespace jpeg{

    struct EntropyChannelOutput{
        std::vector<uint8_t> data;
    };

    class EntropyEncoder{
    public:
        EntropyChannelOutput encode(QuantisedChannelOutput const& input) const;
    protected:
        virtual EntropyChannelOutput applyEncoding(QuantisedChannelOutput const& input) const = 0;
    };

    class HuffmanEncoder : public EntropyEncoder{
    protected:
        EntropyChannelOutput applyEncoding(QuantisedChannelOutput const& input) const override;
    };

}

#endif