#include "..\inc\discrete_cosine_transform.hpp"

jpeg::DCTChannelOutput jpeg::DiscreteCosineTransformer::transform(ColourMappedBlock::ChannelBlock const& inputChannel) const{
    std::array<int8_t, BlockGrid::blockElements> offsetChannelData = applyOffset(inputChannel);
    return applyTransform(offsetChannelData);
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::DiscreteCosineTransformer::inverseTransform(DCTChannelOutput  const& inputChannel) const{
    std::array<int8_t, BlockGrid::blockElements> offsetChannelData = applyInverseTransform(inputChannel);
    return removeOffset(offsetChannelData);
}

std::array<int8_t, jpeg::BlockGrid::blockElements> jpeg::DiscreteCosineTransformer::applyOffset(ColourMappedBlock::ChannelBlock const& input) const{
    std::array<int8_t, BlockGrid::blockElements> offsetData;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        offsetData[i] = input[i] - 128;
    }
    return offsetData;
}

jpeg::ColourMappedBlock::ChannelBlock jpeg::DiscreteCosineTransformer::removeOffset(std::array<int8_t, BlockGrid::blockElements> const& input) const{
    ColourMappedBlock::ChannelBlock output;
    for (size_t i = 0 ; i < BlockGrid::blockElements ; ++i){
        output[i] = input[i] + 128;
    }
    return output;
}

jpeg::DCTChannelOutput jpeg::NaiveCosineTransformer::applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const{
    DCTChannelOutput output;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
            float const scaleFactor = 0.25f * ((u == 0) ? 1/std::sqrt(2.0f) : 1) * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
            float accumulator = 0;
            for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
                for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
                    accumulator += inputChannel[x + y * BlockGrid::blockSize] 
                        * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f)
                        * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f);
                }
            }
            output.data[u + v * BlockGrid::blockSize] = scaleFactor * accumulator;
        }
    }
    return output;
}

std::array<int8_t, jpeg::BlockGrid::blockElements> jpeg::NaiveCosineTransformer::applyInverseTransform(DCTChannelOutput  const& inputChannel) const{
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
    return offsetChannelData;
}

jpeg::DCTChannelOutput jpeg::SeparatedDiscreteCosineTransformer::applyTransform(std::array<int8_t, BlockGrid::blockElements> const& inputChannel) const{
    std::array<float, BlockGrid::blockElements> rowDCT;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        apply1DTransformRow(inputChannel.data(), rowDCT.data() + u * BlockGrid::blockSize, u);
    }

    DCTChannelOutput output;
    for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
        apply1DTransformCol(rowDCT.data(), output.data.data(), v);
    }
    return output;
}

std::array<int8_t, jpeg::BlockGrid::blockElements> jpeg::SeparatedDiscreteCosineTransformer::applyInverseTransform(DCTChannelOutput  const& inputChannel) const{
    std::array<float, BlockGrid::blockElements> rowInverseDCT;
    for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
        apply1DInverseTransformRow(inputChannel.data.data(), rowInverseDCT.data() + x * BlockGrid::blockSize, x);
    }

    std::array<int8_t, BlockGrid::blockElements> offsetChannelData;
    for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
        apply1DInverseTransformCol(rowInverseDCT.data(), offsetChannelData.data(), y);
    }
    return offsetChannelData;
}

void jpeg::SeparatedDiscreteCosineTransformer::apply1DTransformRow(int8_t const* src, float* dest, uint8_t u) const{
    float const scaleFactor = 0.5f * ((u == 0) ? 1/std::sqrt(2.0f) : 1);
    for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
        float accumulator = 0;
        for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
            accumulator += std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f) * src[x + y * BlockGrid::blockSize];
        }
        dest[y] = scaleFactor * accumulator;
    }
}

void jpeg::SeparatedDiscreteCosineTransformer::apply1DTransformCol(float const* src, float* dest, uint8_t v) const{
    float const scaleFactor = 0.5f * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
    float* const output = dest + v * BlockGrid::blockSize;
    for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
        float accumulator = 0;
        for (size_t y = 0 ; y < BlockGrid::blockSize ; ++y){
            accumulator += std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f) * src[y + u * BlockGrid::blockSize];
        }
        output[u] = scaleFactor * accumulator;
    }
}

void jpeg::SeparatedDiscreteCosineTransformer::apply1DInverseTransformRow(float const* src, float* dest, uint8_t x) const{
    for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
        float accumulator = 0;
        for (size_t u = 0 ; u < BlockGrid::blockSize ; ++u){
            float const scaleFactor = 0.5f * ((u == 0) ? 1/std::sqrt(2.0f) : 1);
            accumulator += scaleFactor * std::cos((2.0f * x + 1) * u * std::numbers::pi_v<float> / 16.0f) * src[u + v * BlockGrid::blockSize];
        }
        dest[v] = accumulator;
    }
}
void jpeg::SeparatedDiscreteCosineTransformer::apply1DInverseTransformCol(float const* src, int8_t* dest, uint8_t y) const{    
    int8_t* const output = dest + y * BlockGrid::blockSize;
    for (size_t x = 0 ; x < BlockGrid::blockSize ; ++x){
        float accumulator = 0;
        for (size_t v = 0 ; v < BlockGrid::blockSize ; ++v){
            float const scaleFactor = 0.5f * ((v == 0) ? 1/std::sqrt(2.0f) : 1);
            accumulator += scaleFactor * std::cos((2.0f * y + 1) * v * std::numbers::pi_v<float> / 16.0f) * src[v + x * BlockGrid::blockSize];
        }
        int8_t const maxInt{127};
        int8_t const minInt{-128};
        if (accumulator < minInt){
            output[x] = minInt;
        }
        else if (accumulator > maxInt){
            output[x] = maxInt;
        }
        else{
            output[x] = accumulator;
        }
    }
}