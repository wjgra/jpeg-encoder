#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"
#include "..\inc\colour_mapping.hpp"


namespace jpeg{

    class BlockGenerator{
    public:
    };
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
        Encoder(BitmapImage const& inputImage,
                ColourMapper const& colourMapper,
                BlockGenerator const& blockGenerator,
                DiscreteCosineTransformer const& discreteCosineTransform,
                Quantiser const& quantisation,
                EntropyEncoder const& entropyEncoder);
    public:
        bool saveJPEGToFile(std::string const& savePath);
        JPEGImage getJPEGImageData();
        ColourMappedImageData temp; // to delete - demonstrates colour mapping
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
        JPEGEncoder(BitmapImage const& inputImage);
    };
};

#endif