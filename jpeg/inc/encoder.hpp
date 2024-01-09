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
    protected:
        Encoder(BitmapImageRGB const& inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransformer,
                Quantiser const& quantiser,
                EntropyEncoder const& entropyEncoder);
    public:
        bool saveJPEGToFile(std::string const& savePath);
        JPEGImage getJPEGImageData();
    private:
        JPEGImage jpegImageData;
    };

    class JPEGEncoder : public Encoder{
    public:
        JPEGEncoder(BitmapImageRGB const& inputImage, int quality = 50);
    };
}

#endif