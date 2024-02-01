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

/* This is similar to the encode-decode example, but has been adapted to make use of the HTML interface
   defined in template.html. In Emscripten, the main loop is simulated by throwing a JavaScript exception, 
   then periodically calling a callback function. The difficulty is that this functions like a goto
   statement, so any automatic variables declared at the scope of emscripten_set_main_loop are not destroyed.
   Somewhat ironically, the exception is the string 'unwind', though stack unwinding fails to occur!
   
   To prevent the window being destroyed and recreated every time an encode-decode function is called, the 
   Window object is global. Similarly, to prevent images stored in the simulated filesystem being loaded
   multiple times (and possibly not freed due to GC at the C++/JS boundary, the input bitmaps are global too).
*/

  
//   Compiles with no warnings:
//   emcc examples/web-app.cpp common/*.cpp jpeg/src/*.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic 
//   -std=c++20 -sUSE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" 
//   --preload-file img-cc/ -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=[_main,_malloc,_free] -O3
//
//   DO NOT BUILD FOR NON-EMSCRIPTEN TARGETS


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

Window window(8, 8, "BMP-to-JPEG");

// Global to allow re-use by downloader
jpeg::JPEGImage outputJpeg;

void encodeDecodeImage(jpeg::BitmapImageRGB const& inputBmp, int quality = 80){   
    if (inputBmp.width == 0 || inputBmp.height == 0){
        return;
    }

    window.resize(2 * inputBmp.width, inputBmp.height);
    window.setTitle("BMP-to-JPEG");
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window.getWindow(), -1, 0);
    if (!window.getWindow() || !renderer) return;
    
    // Display pre-encoding image in left half of window
    SDL_Texture* inputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, inputBmp.width, inputBmp.height);
    SDL_UpdateTexture(inputBmpTexture, nullptr, inputBmp.data.data(), inputBmp.width * 3);
    SDL_RenderClear(renderer);
    SDL_Rect leftHalf = {.x = 0, .y = 0,  .w = inputBmp.width, .h = inputBmp.height};
    SDL_RenderCopy(renderer, inputBmpTexture, nullptr, &leftHalf);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(inputBmpTexture);

    // Encode image as JPEG
    auto tStart = std::chrono::high_resolution_clock::now();
    jpeg::BaselineEncoder enc(inputBmp, outputJpeg, quality);
    auto tEnd = std::chrono::high_resolution_clock::now();

    auto timeToEncode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    std::string updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms)");
    window.setTitle(updatedTitle);

    // Decode encoded JPEG image
    jpeg::BitmapImageRGB outputBmp;
    tStart = std::chrono::high_resolution_clock::now();
    jpeg::BaselineDecoder dec(outputJpeg, outputBmp, quality);
    tEnd = std::chrono::high_resolution_clock::now();

    auto timeToDecode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms, decode: ")
        + std::to_string(timeToDecode) + std::string(" ms)");
    window.setTitle(updatedTitle);
    
    // Display post-encoding image in right half of window
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.data.data(), outputBmp.width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.width , .y = 0,  .w = outputBmp.width, .h = outputBmp.height};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(outputBmpTexture);

    std::cout << "BMP size: " << inputBmp.fileSize << "B | JPEG size: " << outputJpeg.fileSize 
              << "B | Compression ratio: " << (outputJpeg.fileSize == 0 ? std::string("N/A") : std::to_string(double(inputBmp.fileSize)/double(outputJpeg.fileSize))) 
              << " | Encode time: " << std::to_string(timeToEncode) + std::string(" ms | Decode time: ") << std::to_string(timeToDecode)
              << " ms\n";
    SDL_DestroyRenderer(renderer);    
}

/* Prevents re-loading with every encode/decode */
jpeg::BitmapImageRGB leclercBmp("img-cc/leclerc-ferrari.bmp");
jpeg::BitmapImageRGB matterhornBmp("img-cc/matterhorn.bmp");
jpeg::BitmapImageRGB saturnBmp("img-cc/cassini-saturn.bmp");

extern "C" {
    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageLeclerc(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage(leclercBmp, quality);
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageMatterhorn(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage(matterhornBmp, quality);
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageSaturn(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage(saturnBmp, quality);
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    /* Prevents user having to re-upload the image if the quality value is changed */
    jpeg::BitmapImageRGB lastUploadedImage;

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImagePreviouslyUploaded(int quality){
        emscripten_cancel_main_loop();
        encodeDecodeImage(lastUploadedImage, quality);
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    EMSCRIPTEN_KEEPALIVE void uploadedImageCallback(std::string const &filename, std::string const &mime_type, std::string_view buffer, void* args){
        std::cout << filename << "(" << mime_type << ") uploaded by user\n";
        lastUploadedImage = jpeg::BitmapImageRGB(reinterpret_cast<uint8_t const*>(buffer.data()), buffer.size());
        emscripten_cancel_main_loop();
        int* mQuality = reinterpret_cast<int*>(args);
        encodeDecodeImage(lastUploadedImage, *mQuality);
        delete mQuality;
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageUpload(int quality){
        int* arg = new int(quality);
        emscripten_browser_file::upload(".bmp", uploadedImageCallback, reinterpret_cast<void*>(arg));
        //emscripten_browser_file::upload(".bmp", uploadedImageCallback, reinterpret_cast<void*>(&quality));
    }

    EMSCRIPTEN_KEEPALIVE void downloadEncodedImage(){
        assert(outputJpeg.supportsSaving);
        std::string filename = "out.jpg";
        std::string mimeType = "image/jpeg";
        emscripten_browser_file::download(filename.c_str(), mimeType.c_str(), reinterpret_cast<char const*>(outputJpeg.compressedImageData.getDataPtr()), outputJpeg.compressedImageData.getSize());
    }
}

EMSCRIPTEN_KEEPALIVE int main(){
    SDL_Init(SDL_INIT_VIDEO);
    encodeDecodeImage(matterhornBmp);
    emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    SDL_Quit();
    return EXIT_SUCCESS;
}