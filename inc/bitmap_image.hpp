#ifndef _JPEG_BITMAP_IMAGE_HPP_
#define _JPEG_BITMAP_IMAGE_HPP_

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <SDL.h>

namespace jpeg{
    /* Stores a 24-bit bitmap image (8-bits for RGB, no alpha channel) */
    struct BitmapImage{
        BitmapImage() : width{0}, height{0} {};
        BitmapImage(std::string const& path); // load from file
        uint16_t width, height;
        struct PixelData{
            uint8_t r, g, b;   
        };
        std::vector<PixelData> data;
    };
};

#endif