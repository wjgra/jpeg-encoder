#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#include "..\inc\window.hpp"
#include "..\inc\encoder.hpp"
// #include "..\inc\decoder.hpp"

void mainLoop(Window& window){
    bool quit = false;
    while (!quit){
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.scancode){
                        case SDL_SCANCODE_F11:
                            window.toggleFullScreen();
                            break;
                        /* case SDL_SCANCODE_ESCAPE:
                            quitApp();
                            break; */
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
    jpeg::BitmapImage bmp("img\\leclerc.bmp"); // load from file
    if (bmp.width == 0 || bmp.height == 0){
        return EXIT_FAILURE;
    }
    //jpeg::Encoder enc(bmp);
    /* jpeg::JPEGImage jpeg = enc.getJPEGImageData();
    jpeg::Decoder dec(jpeg);
    jpeg::BitmapImage outputBmp = dec.getBitmapImageData(); */

    Window window(2 * bmp.width, bmp.height, "BMP-to-JPEG");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Renderer* renderer = SDL_CreateRenderer(window.getWindow(), -1, 0);
    if (!window.getWindow() || !renderer) return EXIT_FAILURE;
    
    SDL_Texture* preEncodingImage = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, bmp.width, bmp.height);
    SDL_UpdateTexture(preEncodingImage, nullptr, bmp.data.data(), bmp.width * 3);
    SDL_RenderClear(renderer);
    SDL_Rect leftHalf = {.x = 0, .y = 0,  .w = bmp.width, .h = bmp.height};
    SDL_RenderCopy(renderer, preEncodingImage, nullptr, &leftHalf);
    SDL_RenderPresent(renderer);

    // Display post-encoding image in right half

    mainLoop(window);
    
    return EXIT_SUCCESS;
}