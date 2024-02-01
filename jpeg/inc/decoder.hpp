#ifndef _JPEG_DECODER_HPP_
#define _JPEG_DECODER_HPP_

#include <string>

#include "jpeg_image.hpp"
#include "bitmap_image.hpp"
#include "block_grid.hpp"
#include "colour_mapping.hpp"
#include "discrete_cosine_transform.hpp"
#include "quantiser.hpp"
#include "entropy_encoder.hpp"
#include "markers.hpp"

namespace jpeg{
    class Decoder{
    public:
        Decoder(JPEGImage inputImage,
                BitmapImageRGB& outputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder);
    private:
        void decodeHeader(BitStream const& inputStream, BitStreamReadProgress& readProgress, BitmapImageRGB& outputImage) const;
    };

    class BaselineDecoder : public Decoder{
    public:
        BaselineDecoder(JPEGImage const& inputImage, BitmapImageRGB& outputImage, int quality = 50);
    };
};

#endif