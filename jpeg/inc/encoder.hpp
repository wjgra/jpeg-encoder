#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <iostream>
#include <utility>
#include <string>
#include <chrono>
#include <memory>

#include "bitmap_image.hpp"
#include "jpeg_image.hpp"
#include "block_grid.hpp"
#include "colour_mapping.hpp"
#include "discrete_cosine_transform.hpp"
#include "quantiser.hpp"
#include "entropy_encoder.hpp"
#include "markers.hpp"

namespace jpeg{
    class Encoder{
    public:
        Encoder(Encoder const&) = delete;
        Encoder(Encoder const&&) = delete;
        Encoder& operator=(Encoder const&) = delete;
        Encoder& operator=(Encoder const&&) = delete;
        ~Encoder() = default;
        Encoder(std::unique_ptr<ColourMapper> colourMapper,
                       std::unique_ptr<DiscreteCosineTransformer> discreteCosineTransformer,
                       std::unique_ptr<Quantiser> quantiser,
                       std::unique_ptr<EntropyEncoder> entropyEncoder);
        void encode(BitmapImageRGB const& inputImage, JPEGImage& outputImage);
        void decode(JPEGImage inputImage, BitmapImageRGB& outputImage);
    private:
        void encodeHeader(BitmapImageRGB const& inputImage, BitStream& outputStream, std::unique_ptr<Quantiser> const& quantiser, std::unique_ptr<EntropyEncoder> const& entropyEncoder) const;
        void decodeHeader(BitStream const& inputStream, BitStreamReadProgress& readProgress, BitmapImageRGB& outputImage) const;
        bool virtual supportsSaving() const = 0;
    private:
        std::unique_ptr<ColourMapper> m_colourMapper;
        std::unique_ptr<DiscreteCosineTransformer> m_discreteCosineTransformer;
        std::unique_ptr<Quantiser> m_quantiser;
        std::unique_ptr<EntropyEncoder> m_entropyEncoder;
    };

    class BaselineEncoder final : public Encoder{
    public:
        BaselineEncoder(int quality) : Encoder(std::make_unique<RGBToYCbCrMapper>(), 
                                                             std::make_unique<SeparatedDiscreteCosineTransformer>(), 
                                                             std::make_unique<Quantiser>(quality), 
                                                             std::make_unique<HuffmanEncoder>()){
        }
    private:
        bool supportsSaving() const override {return true;}
    };
}
#endif