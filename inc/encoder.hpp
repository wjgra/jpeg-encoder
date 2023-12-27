#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>
#include <array>
#include <vector>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"


namespace jpeg{

    struct ColourMappedImageData{
        uint16_t width, height;
        std::vector<std::array<uint8_t, 3>> data;
    };

    class ColourMapper{
    public:
        virtual ColourMappedImageData map(BitmapImage const& inputImage) const = 0;
    };

    class RGBToRGBMapper : public ColourMapper{
    public:
        virtual ColourMappedImageData map(BitmapImage const& bmp) const override;
    };

    class RGBToYCbCrMapper : public ColourMapper{
    public:
       virtual ColourMappedImageData map(BitmapImage const& bmp) const override;
    };

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
        ColourMappedImageData temp;
    private:
        JPEGImage jpegImageData;
        // references to encoder components
        ColourMapper const& colourMapper;
        BlockGenerator const& blockGenerator;
        DiscreteCosineTransformer const& discreteCosineTransform;
        Quantiser const& quantisation;
        EntropyEncoder const& entropyEncoder;
    };

    class JPEGEncoder : public Encoder{
    public:
        JPEGEncoder(BitmapImage const& inputImage);
    };
};

#endif