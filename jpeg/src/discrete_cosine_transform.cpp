#include "..\inc\discrete_cosine_transform.hpp"

jpeg::DCTChannelOutput jpeg::DiscreteCosineTransformer::transform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    return applyTransform(inputChannel); // Issue: move transforms to separate functions
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::DiscreteCosineTransformer::inverseTransform(DCTChannelOutput  const& inputChannel) const{
    return applyInverseTransform(inputChannel);
}

jpeg::DCTChannelOutput jpeg::NaiveCosineTransformer::applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    std::array<int8_t, BlockGrid::blockElements> offsetChannelData;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        offsetChannelData[i] = inputChannel[i] - 128;
    }
    DCTChannelOutput output;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
            float const scaleFactor = 0.25f * ((u == 0) ? 1/std::sqrt(2.0f) : 1) * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
            float accumulator = 0;
            for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
                for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
                    accumulator += offsetChannelData[x + y * BlockGrid::blockSize] 
                        * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f)
                        * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f);
                }
            }
            output.data[u + v * BlockGrid::blockSize] = scaleFactor * accumulator;
        }
    }
    return output;
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::NaiveCosineTransformer::applyInverseTransform(DCTChannelOutput  const& inputChannel) const{
    std::array<int8_t, BlockGrid::blockElements> offsetChannelData;
    for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
        for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
            float accumulator = 0;
            for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
                for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
                    float const scaleFactor = 0.25f * ((u == 0) ? 1/std::sqrt(2.0f) : 1) * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
                    accumulator += scaleFactor * inputChannel.data[u + v * BlockGrid::blockSize] 
                        * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f)
                        * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f);
                }
            }
            int8_t const maxInt{127};
            int8_t const minInt{-128};
            if (accumulator < minInt){
                offsetChannelData[x + y * BlockGrid::blockSize] = minInt;
            }
            else if (accumulator > maxInt){
                offsetChannelData[x + y * BlockGrid::blockSize] = maxInt;
            }
            else{
                offsetChannelData[x + y * BlockGrid::blockSize] = accumulator;
            }
        }
    }
    ColourMappedBlock::ChannelBlock channelData;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        channelData[i] = offsetChannelData[i] + 128;
    }
    return channelData;
}

jpeg::DCTChannelOutput jpeg::NestedCosineTransformer::applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    std::array<int8_t, BlockGrid::blockElements> offsetChannelData;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        offsetChannelData[i] = inputChannel[i] - 128;
    }
    DCTChannelOutput output;

    for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
        float const outerScaleFactor = 0.5f * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
        std::array<float, BlockGrid::blockSize> outerAccumulator{};
        for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){

            /* std::array<float, BlockGrid::blockSize> innerDCT{};
            for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
                float const innerScaleFactor = 0.5f * ((u == 0) ? 1/std::sqrt(2.0f) : 1);
                float accumulator = 0;
                
                    for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
                        accumulator += offsetChannelData[x + y * BlockGrid::blockSize] 
                                    * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f);
                    }
                innerDCT[u] = innerScaleFactor * accumulator;
            } */

            std::array<int8_t, BlockGrid::blockSize> rowInput{};
            std::copy(offsetChannelData.begin() + y * BlockGrid::blockSize, offsetChannelData.begin() + (y + 1) * BlockGrid::blockSize, rowInput.begin());
            auto innerDCT = apply1DTransform(rowInput);

            // vector operation
            for (int i = 0 ; i < 8 ; ++i){
                outerAccumulator[i] += innerDCT[i] * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f);
            }

        }
        // vector operation
        for (int i = 0 ; i < 8 ; ++i){
            output.data[i + v * BlockGrid::blockSize] = outerScaleFactor * outerAccumulator[i];
        }

    }
    return output;
}

std::array<float, jpeg::BlockGrid::blockSize> jpeg::NestedCosineTransformer::apply1DTransform(std::array<int8_t, BlockGrid::blockSize> const& input) const{
    std::array<float, jpeg::BlockGrid::blockSize> output;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        float const scaleFactor = 0.5f * ((u == 0) ? 1/std::sqrt(2.0f) : 1);
        float accumulator = 0;
        for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
            accumulator += input[x] * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f);
        }
        output[u] = scaleFactor * accumulator;
    }
    return output;
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::NestedCosineTransformer::applyInverseTransform(DCTChannelOutput  const& inputChannel) const{
    /* Unimplemented */
    return ColourMappedBlock::ChannelBlock();
}