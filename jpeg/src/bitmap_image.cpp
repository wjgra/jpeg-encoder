#include "..\inc\bitmap_image.hpp"

jpeg::BitmapImageRGB::BitmapImageRGB() : width{0}, height{0} {};

jpeg::BitmapImageRGB::BitmapImageRGB(uint16_t w, uint16_t h) : width{w}, height{h} {
    data.resize(width * height);
}

jpeg::BitmapImageRGB::BitmapImageRGB(SDL_Surface* image){
    if(!init(image)){
        return; 
    }
}

jpeg::BitmapImageRGB::BitmapImageRGB(std::string const& loadPath){
    SDL_Surface* image = SDL_LoadBMP(loadPath.c_str());   
    if (!image){
        std::cout << "Failed to load image at " << loadPath << " (" << SDL_GetError() << ")\n";
        width = 0; height = 0; data.clear();
        return;
    }
    if(!init(image)){
        return; 
    }
    SDL_FreeSurface(image);
    fileSize = std::filesystem::file_size(loadPath.c_str());
}

jpeg::BitmapImageRGB::BitmapImageRGB(uint8_t const* buffer, int len){
    SDL_RWops* stream = SDL_RWFromMem(reinterpret_cast<void*>(const_cast<uint8_t*>(buffer)), len);
    fileSize = len;
    if (!stream){
        std::cout << "Failed to create image stream (" << SDL_GetError() << ")\n";
        width = 0; height = 0; data.clear();
        return;   
    }
    SDL_Surface* image = SDL_LoadBMP_RW(stream, SDL_TRUE);
    if(!init(image)){
        return; 
    }
    SDL_FreeSurface(image);
}

bool jpeg::BitmapImageRGB::init(SDL_Surface* image){
    if (!image){
        std::cout << "Failed to load image (" << SDL_GetError() << ")\n";
        width = 0; height = 0; data.clear();
        return false;
    }
    width = image->w;
    height = image->h;
    /* if (fileSize == 0){
        auto len = width * height * 3;
        fileSize = len + 54 + (len % 4) ? 0 : (4 - (len % 4)); // Approximate BMP file size, assuming 54 byte header
    } */
    data.clear();
    for (int i = 0 ; i < height; ++i){
        for (int j = 0 ; j < width; ++j){
            uint32_t* pixel = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(image->pixels) + i * image->pitch + 3 * j);
            PixelData temp;
            SDL_GetRGB(*pixel, image->format, &(temp.r), &(temp.g), &(temp.b));
            data.push_back(temp);
        }
    }
    return true;
}