#ifndef _JPEG_ENCODER_HPP_
#define _JPEG_ENCODER_HPP_

#include <string>
#include <vector>
#include <iterator>

#include "..\inc\bitmap_image.hpp"
#include "..\inc\jpeg_image.hpp"
#include "..\inc\colour_mapping.hpp"


namespace jpeg{

    class BlockGrid{
    public:
        BlockGrid(BitmapImageRGB const& input) : imageData{input}{};
        uint16_t const static blockSize = 8;
        // input iterator only
        struct BlockIterator{
            using difference_type = std::ptrdiff_t;
            using value_type = const BitmapImageRGB::PixelData;
            using pointer = value_type *;
            using reference = value_type &;
            using iterator_category = std::input_iterator_tag;
        private:
            pointer ptr;/* , sentinel;
            static_assert(std::sentinel_for<decltype(sentinel), decltype(ptr)>); */
            uint16_t gridWidth, gridHeight;
        public:
            explicit BlockIterator() = default;
            BlockIterator(pointer p/* , pointer s */, uint16_t w, uint16_t h) : ptr{p}/* , sentinel{s}  */, gridWidth{w}, gridHeight{h}{};
            reference operator*() const {return *ptr;};
            BlockIterator& operator++() {ptr += blockSize * gridWidth; return *this;};
            BlockIterator operator++(int) {auto temp = *this; ++(*this); return temp;};

            pointer operator->() {return ptr;};
            friend bool operator==(BlockIterator const& a, BlockIterator const& b){return a.ptr == b.ptr;};
            friend bool operator!=(BlockIterator const& a, BlockIterator const& b){return a.ptr != b.ptr;};
        };
    public:
        BlockIterator begin(){return BlockIterator(imageData.data.data(), imageData.width, imageData.height);};
        BlockIterator end(){return BlockIterator(imageData.data.data() + blockSize * imageData.width * (1 + imageData.height / blockSize), 
                                             imageData.width, imageData.height);};
    private:
        BitmapImageRGB const& imageData;

        static_assert(std::input_iterator<BlockIterator>);
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