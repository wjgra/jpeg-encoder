#include "..\inc\bitmap_image.hpp"

jpeg::BitmapImage::BitmapImage(std::string const& loadPath){
    SDL_Surface* image = SDL_LoadBMP(loadPath.c_str());   
    if (!image){
        std::cout << "Failed to load image at " << loadPath << " (" << SDL_GetError() << ")\n";
        width = 0; height = 0; data.clear();
        return;
    }

    width = image->w;
    height = image->h;
    data.clear();

    for (int i = 0 ; i < height; ++i){
        for (int j = 0 ; j < width; ++j){
            uint32_t* pixel = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(image->pixels) + i * image->pitch + 3 * j);
            PixelData temp;
            SDL_GetRGB(*pixel, image->format, &(temp.r), &(temp.g), &(temp.b));
            data.push_back(temp);
        }
    }
    SDL_FreeSurface(image);
}