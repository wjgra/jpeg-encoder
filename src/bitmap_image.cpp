#include "..\inc\bitmap_image.hpp"

jpeg::BitmapImage::BitmapImage(std::string const& path){
    SDL_Surface* image = SDL_LoadBMP(path.c_str());   

    if (!image){
        std::cout << "Failed to load image at " << path << " (" << SDL_GetError() << ")\n";
        width = 0; height = 0; data.clear();
        return;
    }

    width = image->w;
    height = image->h;
    data.clear();
    /* for (int i = 0 ; i < image->pitch * height; ++i){
        PixelData temp;
        SDL_GetRGB(*(static_cast<uint32_t*>(image->pixels)+i), image->format, &(temp.r), &(temp.g), &(temp.b));
        data.push_back(temp);
    } */

    for (int i = 0 ; i < height; ++i){
        for (int j = 0 ; j < width; ++j){
            uint32_t* pixel = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(image->pixels) + i * image->pitch + j *3);
            PixelData temp;
            SDL_GetRGB(*pixel, image->format, &(temp.r), &(temp.g), &(temp.b));
            data.push_back(temp);
        }
    }
    // copy data to struct

    SDL_FreeSurface(image);
}