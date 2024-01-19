#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"
#include "..\inc\block_grid.hpp"
#include "..\inc\colour_mapping.hpp"
#include "..\inc\discrete_cosine_transform.hpp"
#include "..\inc\quantiser.hpp"
#include "..\inc\entropy_encoder.hpp"

namespace jpeg{
    class Encoder{
    public:
        Encoder(BitmapImageRGB const& inputImage,
                JPEGImage& outputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder);
        ~Encoder(){std::cout << "Encoder dtor\n";};
    };

    class JPEGEncoder : public Encoder{
    public:
        JPEGEncoder(BitmapImageRGB const& inputImage, JPEGImage& outputImage, int quality = 50);
    };
}

#endif