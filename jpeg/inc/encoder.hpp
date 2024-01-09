#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"
#include "..\inc\block_grid.hpp"
#include "..\inc\colour_mapping.hpp"

namespace jpeg{

    class DiscreteCosineTransformer{
    public:
    };
    class Quantiser{
    public:
    };
    class EntropyEncoder{
    public:
    };

    class Encoder{
    protected:
        Encoder(BitmapImageRGB const& inputImage,
                ColourMapper const& colourMapper,
                DiscreteCosineTransformer const& discreteCosineTransform,
                Quantiser const& quantisation,
                EntropyEncoder const& entropyEncoder);
    public:
        bool saveJPEGToFile(std::string const& savePath);
        JPEGImage getJPEGImageData();
    private:
        JPEGImage jpegImageData;
        // references to encoder components
        /* ColourMapper const& colourMapper;
        BlockGenerator const& blockGenerator;
        DiscreteCosineTransformer const& discreteCosineTransform;
        Quantiser const& quantisation;
        EntropyEncoder const& entropyEncoder; */
    };

    class JPEGEncoder : public Encoder{
    public:
        JPEGEncoder(BitmapImageRGB const& inputImage);
    };
};

#endif