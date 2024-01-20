#ifndef _JPEG_DECODER_HPP_
#define _JPEG_DECODER_HPP_

#include <string>

#include "..\inc\jpeg_image.hpp"
#include "..\inc\bitmap_image.hpp"
#include "..\inc\block_grid.hpp"
#include "..\inc\colour_mapping.hpp"
#include "..\inc\discrete_cosine_transform.hpp"
#include "..\inc\quantiser.hpp"
#include "..\inc\entropy_encoder.hpp"

namespace jpeg{
    class Decoder{
    public:
        Decoder(JPEGImage inputImage,
                BitmapImageRGB& outputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder);
    };

    class JPEGDecoder : public Decoder{
    public:
        JPEGDecoder(JPEGImage const& inputImage, BitmapImageRGB& outputImage, int quality = 50);
    };
};

#endif