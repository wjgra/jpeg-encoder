#ifndef _JPEG_BITMAP_IMAGE_HPP_
#define _JPEG_BITMAP_IMAGE_HPP_

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include <SDL.h> // Issue: SDL dependency is used for reading BMPs

namespace jpeg{

    /* Stores a 24-bit RGB bitmap image (8-bits for R, G and B; no alpha channel) */
    struct BitmapImageRGB{
        BitmapImageRGB();
        BitmapImageRGB(uint16_t w, uint16_t h);
        BitmapImageRGB(SDL_Surface* image);
        BitmapImageRGB(std::string const& loadPath);
        BitmapImageRGB(uint8_t const* buffer, int len);
        ~BitmapImageRGB(){std::cout << "Bitmap dtor\n";};
        struct PixelData{
            uint8_t r, g, b;   
        };
        uint16_t width, height;
        std::uintmax_t fileSize;
        std::vector<PixelData> data;
    private:
        bool init(SDL_Surface* image);
    };
}

#endif