#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#include <chrono>
#include <vector>
#include <string>

#include "..\common\window.hpp"
#include "..\jpeg\inc\encoder.hpp"
#include "..\jpeg\inc\decoder.hpp"

bool mainLoop(Window& window){
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                return false;
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
    return true;
}

#ifdef __EMSCRIPTEN__
void mainLoopCallback(void* window){
    mainLoop(*static_cast<Window*>(window));
}
#endif

int main(int argc, char *argv[]){
    std::vector<std::string> arguments(argv + 1, argv + argc);
    int qualityValue = 80;
    int scale = 1;
    std::string inputBmpImagePath = "img/leclerc.bmp";
    for(auto arg = arguments.begin() ; arg != arguments.end() ; ++arg){
        if (strcmp(arg->c_str(), "-q") == 0){
            if (arg != arguments.end() - 1){
                ++arg;
                qualityValue = std::stoi(std::string(*arg));
            }
        }
        else if (strcmp(arg->c_str(), "-i") == 0){
            if (arg != arguments.end() - 1){
                ++arg;
                inputBmpImagePath = *arg;
            }
        }
        else if (strcmp(arg->c_str(), "-s") == 0){
            if (arg != arguments.end() - 1){
                ++arg;
                scale = std::stoi(std::string(*arg));
            }
        }
    }

    // Load test image
    
    jpeg::BitmapImageRGB inputBmp(inputBmpImagePath);
    
    if (inputBmp.width == 0 || inputBmp.height == 0){
        return EXIT_FAILURE;
    }

    Window window(2 * inputBmp.width * scale, inputBmp.height * scale, "BMP-to-JPEG");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Renderer* renderer = SDL_CreateRenderer(window.getWindow(), -1, 0);
    if (!window.getWindow() || !renderer) return EXIT_FAILURE;
    
    // Display pre-encoding image in left half of window
    SDL_Texture* inputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, inputBmp.width, inputBmp.height);
    SDL_UpdateTexture(inputBmpTexture, nullptr, inputBmp.data.data(), inputBmp.width * 3);
    SDL_RenderClear(renderer);
    SDL_Rect leftHalf = {.x = 0, .y = 0,  .w = inputBmp.width * scale, .h = inputBmp.height * scale};
    SDL_RenderCopy(renderer, inputBmpTexture, nullptr, &leftHalf);
    SDL_RenderPresent(renderer);

    // Encode image as JPEG
    jpeg::JPEGImage outputJpeg;
    auto tStart = std::chrono::high_resolution_clock::now();
    jpeg::JPEGEncoder enc(inputBmp, outputJpeg, qualityValue);
    auto tEnd = std::chrono::high_resolution_clock::now();

    auto timeToEncode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    std::string updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms)");
    SDL_SetWindowTitle(window.getWindow(), updatedTitle.c_str());

    // Decode encoded JPEG image
    jpeg::BitmapImageRGB outputBmp;
    tStart = std::chrono::high_resolution_clock::now();
    jpeg::JPEGDecoder dec(outputJpeg, outputBmp, qualityValue);
    tEnd = std::chrono::high_resolution_clock::now();

    auto timeToDecode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms, decode: ")
        + std::to_string(timeToDecode) + std::string(" ms)");
    SDL_SetWindowTitle(window.getWindow(), updatedTitle.c_str());
    
    // Display post-encoding image in right half of window
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.data.data(), outputBmp.width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.width * scale, .y = 0,  .w = outputBmp.width * scale, .h = outputBmp.height * scale};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);

    #ifndef __EMSCRIPTEN__
    while(mainLoop(window)){}
    #else
    emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, 1);
    #endif
    
    SDL_Quit();
    return EXIT_SUCCESS;
}