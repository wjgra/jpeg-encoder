#include <emscripten.h>
#include "..\common\emscripten_browser_file.h"

#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

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

void mainLoopCallback(void* window){
    mainLoop(*static_cast<Window*>(window));
}

int encodeDecodeImage(jpeg::BitmapImageRGB const& inputBmp, int quality = 80){   
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
    jpeg::JPEGEncoder enc(inputBmp, quality);
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
    jpeg::JPEGDecoder dec(outputJpeg, quality);
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
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.data.data(), outputBmp.width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.width , .y = 0,  .w = outputBmp.width, .h = outputBmp.height};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);

    std::cout << "BMP size: " << inputBmp.fileSize << "B | JPEG size: " << "XX" << "B | Compression ratio: " << "1.0\n";

    emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);

    SDL_Quit();
    return EXIT_SUCCESS;
}

int encodeDecodeImage(std::string const& path = "img-cc/matterhorn.bmp", int quality = 80){
    jpeg::BitmapImageRGB inputBmp(path);
    return encodeDecodeImage(inputBmp, quality);

    /* if (inputBmp.width == 0 || inputBmp.height == 0){
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
    jpeg::JPEGEncoder enc(inputBmp, quality);
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
    jpeg::JPEGDecoder dec(outputJpeg, quality);
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
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.data.data(), outputBmp.width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.width , .y = 0,  .w = outputBmp.width, .h = outputBmp.height};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);

    std::cout << "Bitmap size: " << fileSize << "B | JPEG size: " << "XX" << "B | Compression ratio: " << "1.0\n";

    emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);

    SDL_Quit();
    return EXIT_SUCCESS; */
}

/* int encodeDecodeImage(int const& address, int len, int quality = 80){
    uint8_t* data = reinterpret_cast<uint8_t*>(address);
    
    auto fileSize = len; // ???
    jpeg::BitmapImageRGB inputBmp(data, len);
    
    encodeDecodeImage(inputBmp, quality);
} */

extern "C" {
    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageLeclerc(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage("img-cc/leclerc-ferrari.bmp", quality);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageMatterhorn(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage("img-cc/matterhorn.bmp", quality);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageSaturn(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage("img-cc/cassini-saturn.bmp", quality);
    }

    jpeg::BitmapImageRGB lastUploadedImage;

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImagePreviouslyUploaded(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage(lastUploadedImage, quality);
    }

    EMSCRIPTEN_KEEPALIVE void uploadedImageCallback(std::string const &filename, std::string const &mime_type, std::string_view buffer, void* args){
        std::cout << filename << "(" << mime_type << ") uploaded by user\n";
        lastUploadedImage = jpeg::BitmapImageRGB(reinterpret_cast<uint8_t const*>(buffer.data()), buffer.size());
        encodeDecodeImage(lastUploadedImage, *reinterpret_cast<int*>(args));
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageUpload(int quality){
        emscripten_browser_file::upload(".bmp", uploadedImageCallback, reinterpret_cast<void*>(&quality));
        emscripten_cancel_main_loop();
    }
}

EMSCRIPTEN_KEEPALIVE int main(){
    return encodeDecodeImage();
}