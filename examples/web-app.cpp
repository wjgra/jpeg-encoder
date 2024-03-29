#include <emscripten.h>
#include "emscripten_browser_file.h"

#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

#include "window.hpp"
#include "encoder.hpp"

/* 
   This is similar to the encode-decode example, but has been adapted to make use of the HTML interface
   defined in template.html. In Emscripten, the main loop is simulated by throwing a JavaScript exception, 
   then periodically calling a callback function. The difficulty is that this functions like a goto
   statement, so any automatic variables declared at the scope of emscripten_set_main_loop are not destroyed.
   Somewhat ironically, the exception is the string 'unwind', though stack unwinding fails to occur!
   
   To prevent the window being destroyed and recreated every time an encode-decode function is called, the 
   Window object is global. Similarly, to prevent images stored in the simulated filesystem being loaded
   multiple times (and possibly not freed due to GC at the C++/JS boundary, the input bitmaps are global too).

   DO NOT BUILD FOR NON-EMSCRIPTEN TARGETS
*/

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
    if (inputBmp.m_width == 0 || inputBmp.height == 0){
        return;
    }

    window.resize(2 * inputBmp.m_width, inputBmp.height);
    window.setTitle("BMP-to-JPEG");
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window.getWindow(), -1, 0);
    if (!window.getWindow() || !renderer) return;
    
    // Display pre-encoding image in left half of window
    SDL_Texture* inputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, inputBmp.m_width, inputBmp.height);
    SDL_UpdateTexture(inputBmpTexture, nullptr, inputBmp.m_imageData.data(), inputBmp.m_width * 3);
    SDL_RenderClear(renderer);
    SDL_Rect leftHalf = {.x = 0, .y = 0,  .w = inputBmp.m_width, .h = inputBmp.height};
    SDL_RenderCopy(renderer, inputBmpTexture, nullptr, &leftHalf);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(inputBmpTexture);

    // Encode image as JPEG
    auto tStart = std::chrono::high_resolution_clock::now();
    jpeg::BaselineEncoder encoder(quality);
    encoder.encode(inputBmp, outputJpeg);
    auto tEnd = std::chrono::high_resolution_clock::now();

    auto timeToEncode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    std::string updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.m_width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms)");
    window.setTitle(updatedTitle);

    // Decode encoded JPEG image
    jpeg::BitmapImageRGB outputBmp;
    tStart = std::chrono::high_resolution_clock::now();
    encoder.decode(outputJpeg, outputBmp);
    tEnd = std::chrono::high_resolution_clock::now();

    auto timeToDecode = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(tEnd - tStart).count();
    updatedTitle = std::string("BMP-to-JPEG (") 
        + std::to_string(inputBmp.m_width) + std::string("x")
        + std::to_string(inputBmp.height) + std::string(", encode: ")
        + std::to_string(timeToEncode) + std::string(" ms, decode: ")
        + std::to_string(timeToDecode) + std::string(" ms)");
    window.setTitle(updatedTitle);
    
    // Display post-encoding image in right half of window
    SDL_Texture* outputBmpTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, outputBmp.m_width, outputBmp.height);
    SDL_UpdateTexture(outputBmpTexture, nullptr, outputBmp.m_imageData.data(), outputBmp.m_width * 3);
    SDL_Rect rightHalf = {.x = inputBmp.m_width , .y = 0,  .w = outputBmp.m_width, .h = outputBmp.height};
    SDL_RenderCopy(renderer, outputBmpTexture, nullptr, &rightHalf);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(outputBmpTexture);

    std::cout << "BMP size: " << inputBmp.m_fileSize << "B | JPEG size: " << outputJpeg.m_fileSize 
              << "B | Compression ratio: " << (outputJpeg.m_fileSize == 0 ? std::string("N/A") : std::to_string(double(inputBmp.m_fileSize)/double(outputJpeg.m_fileSize))) 
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
        int* quality_ptr = reinterpret_cast<int*>(args);
        encodeDecodeImage(lastUploadedImage, *quality_ptr);
        delete quality_ptr;
        emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    }

    EMSCRIPTEN_KEEPALIVE void encodeDecodeImageUpload(int quality){
        int* arg_ptr = new int(quality);
        emscripten_browser_file::upload(".bmp", uploadedImageCallback, reinterpret_cast<void*>(arg_ptr));
    }

    EMSCRIPTEN_KEEPALIVE void downloadEncodedImage(){
        assert(outputJpeg.m_supportsSaving);
        std::string filename = "out.jpg";
        std::string mimeType = "image/jpeg";
        emscripten_browser_file::download(filename.c_str(), mimeType.c_str(), reinterpret_cast<char const*>(outputJpeg.m_compressedImageData.getDataPtr()), outputJpeg.m_compressedImageData.getSize());
    }
}

EMSCRIPTEN_KEEPALIVE int main(){
    SDL_Init(SDL_INIT_VIDEO);
    encodeDecodeImage(matterhornBmp);
    emscripten_set_main_loop_arg(&mainLoopCallback, &window, 0, true);
    SDL_Quit();
    return EXIT_SUCCESS;
}