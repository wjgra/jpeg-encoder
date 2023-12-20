#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#include "..\inc\window.hpp"
#include "..\inc\encoder.hpp"
#include "..\inc\decoder.hpp"

void mainLoop(Window& window){
    while (true){
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    return;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.scancode){
                        case SDL_SCANCODE_F11:
                            window.toggleFullScreen();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

int main(){
    // Load test image
    jpeg::BitmapImage inputBmp("img\\leclerc.bmp");
    if (inputBmp.width == 0 || inputBmp.height == 0){
        return EXIT_FAILURE;
    }

    Window window(2 * inputBmp.width, inputBmp.height, "BMP-to-JPEG");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Renderer* renderer = SDL_CreateRenderer(window.getWindow(), -1, 0);
    if (!window.getWindow() || !renderer) return EXIT_FAILURE;
    
    // Display pre-encoding image in left half of window
    SDL_Texture* inputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, inputBmp.width, inputBmp.height);
    SDL_UpdateTexture(inputBmpTexture, nullptr, inputBmp.data.data(), inputBmp.width * 3);
    SDL_RenderClear(renderer);
    SDL_Rect leftHalf = {.x = 0, .y = 0,  .w = inputBmp.width, .h = inputBmp.height};
    SDL_RenderCopy(renderer, inputBmpTexture, nullptr, &leftHalf);
    SDL_RenderPresent(renderer);

    // Encode image as JPEG
    jpeg::Encoder enc(inputBmp);
    jpeg::JPEGImage outputJpeg = enc.getJPEGImageData();
    jpeg::Decoder dec(outputJpeg);
    // jpeg::BitmapImage outputBmp = dec.getBitmapImageData();

    // Display post-encoding image in right half of window
    // Consider using two windows instead...

    mainLoop(window);
    return EXIT_SUCCESS;
}