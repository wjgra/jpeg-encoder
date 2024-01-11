#include "..\inc\discrete_cosine_transform.hpp"

jpeg::DCTChannelOutput jpeg::DiscreteCosineTransformer::transform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    return applyTransform(inputChannel);
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::DiscreteCosineTransformer::inverseTransform(DCTChannelOutput  const& inputChannel) const{
    return applyInverseTransform(inputChannel);
}

jpeg::DCTChannelOutput jpeg::NaiveDCTTransformer::applyTransform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    ColourMappedBlock::ChannelBlock offsetChannelData;
    for (size_t i = 0 ; i < offsetChannelData.size() ; ++i){
        offsetChannelData[i] = inputChannel[i] - 128;
    }
    DCTChannelOutput output;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
            float scaleFactor = 0.25f * ((u == 0) ? 1/std::sqrt(2.0f) : 1) * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
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

jpeg::ColourMappedBlock::ChannelBlock jpeg::NaiveDCTTransformer::applyInverseTransform(DCTChannelOutput  const& inputChannel) const{
    ColourMappedBlock::ChannelBlock offsetChannelData;
    for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
        for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
            float accumulator = 0;
            for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
                for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
                    float scaleFactor = 0.25f * ((u == 0) ? 1/std::sqrt(2.0f) : 1) * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
                    accumulator += scaleFactor * inputChannel.data[u + v * BlockGrid::blockSize] 
                        * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f)
                        * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f);
                }
            }
            offsetChannelData[x + y * BlockGrid::blockSize] = accumulator;
        }
    }
    ColourMappedBlock::ChannelBlock channelData;
    for (size_t i = 0 ; i < channelData.size() ; ++i){
        channelData[i] = offsetChannelData[i] + 128;
    }
    return channelData;
}

/* To do: implement fast DCTTransformer */