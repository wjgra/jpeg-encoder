#define SDL_MAIN_HANDLED
#include <SDL_main.h>
#include <chrono>

#include "..\common\window.hpp"
#include "..\jpeg\inc\encoder.hpp"
#include "..\jpeg\inc\decoder.hpp"

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

int main(int argc, char *argv[]){
    int qualityValue = 50;
    if (argc > 1){
        qualityValue = std::stoi(std::string(argv[1]));
    }
    // Load test image
    jpeg::BitmapImageRGB inputBmp("img\\leclerc.bmp");
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
    auto tStart = std::chrono::high_resolution_clock::now();
    jpeg::JPEGEncoder enc(inputBmp, qualityValue);
    auto tEnd = std::chrono::high_resolution_clock::now();

    auto timeToEncode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    std::string updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms)");
    SDL_SetWindowTitle(window.getWindow(), updatedTitle.c_str());

    // Decode encoded JPEG image
    jpeg::JPEGImage outputJpeg = enc.getJPEGImageData();
    tStart = std::chrono::high_resolution_clock::now();
    jpeg::JPEGDecoder dec(outputJpeg, qualityValue);
    tEnd = std::chrono::high_resolution_clock::now();

    auto timeToDecode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms, decode: ")
        + std::to_string(timeToDecode) + std::string(" ms)");
    SDL_SetWindowTitle(window.getWindow(), updatedTitle.c_str());


    jpeg::BitmapImageRGB outputBmp = dec.getBitmapImageData();
    
    // Display post-encoding image in right half of window
    // Consider using two windows instead...

    // temp display colour mapped data
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.data.data(), outputBmp.width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.width, .y = 0,  .w = outputBmp.width, .h = outputBmp.height};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);

    mainLoop(window);
    return EXIT_SUCCESS;
}