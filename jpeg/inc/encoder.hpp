#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>
#include <chrono>

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
        Encoder(BitmapImageRGB const& inputImage,
                JPEGImage& outputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder);
    private:
        void encodeHeader(BitmapImageRGB const& inputImage, BitStream& outputStream, Quantiser const& quantiser, EntropyEncoder const& entropyEncoder) const;
        void stuffAlignedBytes(BitStream& outputStream, size_t startOfScanData) const;
    };

    class BaselineEncoder : public Encoder{
    public:
        BaselineEncoder(BitmapImageRGB const& inputImage, JPEGImage& outputImage, int quality = 50);
    };
}

#endif