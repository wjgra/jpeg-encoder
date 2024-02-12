# JPEG Encoding and Decoding Library
## Overview
This is small library that enables encoding of 24-bit bitmap images to baseline sequential JPEGs as specified in [ITU T.81](https://www.w3.org/Graphics/JPEG/itu-t81.pdf) (link to PDF) and decoding such JPEGs to recover an approximation of the original bitmap. Please do not rely on this for anything important, it was primarily a fun learning exercise, and my first foray into the world of image compression. I prioritised 1) fun and 2) trying out C++17/20 features that I hadn't used before over speed, but it's fast enough to use interactively ([link to web-app](http://www.wjgrace.co.uk/projects/jpeg/jpeg.html)).

There are plenty of extensions that I could add at some point in the future (e.g. support for single-channel [i.e. greyscale] image encoding, progressive encoding, arithmetic encoding), but this seems complete enough I'm happy to leave it as it stands for now, critical bugs notwithstanding.

### Usage

Example usage with a baseline encoder:

```
#include "encoder.hpp"

// Load input image
jpeg::BitmapImageRGB inputBmp("path_to_input\my_image.bmp");

// Create a new baseline encoder-decoder
int qualityValue = 75; // Integer quality value for use in encoding, limited to between 1 and 100
jpeg::BaselineEncoder encoder(qualityValue);

// Encode as JPEG
jpeg::JPEGImage outputJpeg;
encoder.encode(inputBmp, outputJpeg);

// Save JPEG to file
outputJpeg.saveToFile("path_to_output\m_image.jpg");

// Decode JPEG to bitmap
jpeg::BitmapImageRGB decodedBmp;
encoder.decoder(outputJpeg, decodedBmp);
    
```

### Extension

The `Encoder` class has been designed to allow for easy extension, using dependency injection to reduce coupling between the individual components of the encoder. In particular, the `Encoder` class contains member `unique_ptr`s to each of the colour mapper, discrete cosine transformer, quantiser and entropy encoder. In this way, custom `Encoder` objects may be created either by passing unique_ptrs directly to the constructor, or by inheritance.

Example extension to use RGB-to-RGB mapping prior to DCT application (not supported for saving in standard JPEG file interchange format) instead of RGB-to-YCbCr mapping:

```
class MyNewEncoder final : public Encoder{
    public:
        MyNewEncoder(int quality) : Encoder(std::make_unique<RGBToRGBMapper>(), 
                                            std::make_unique<SeparatedDiscreteCosineTransformer>(), 
                                            std::make_unique<Quantiser>(quality), 
                                            std::make_unique<HuffmanEncoder>()){
        }
    private:
        bool supportsSaving() const override {return false;}
    };
```

## Dependencies
Bitmap loading is handled by SDL's loadImage function, as I was already using SDL for window creation. It would be a good idea to either write my own bitmap parser at some point, or to use one of the many header-only libraries that already exist for this purpose.

Otherwise, the classes in this library only depend on the C++20 STL.

## Compilation of Example Programs
Both examples compile on my computer with no warnings. Some sample g++ commands are below, though obviously these may differ based on your environment.

### Native Example
Takes a single 24-bit RGB bitmap as input, encodes it as a JPEG, then decodes the compressed JPEG version for display side-by-side with the original. For small images, the scale parameter may be used to 'blow them up' such that they are more clearly visible. 

Usage (parameters may be provided in any order):
```
.\jpeg.exe 
    -i [PATH_TO_INPUT_BMP] (the input bitmap file for encoding)
    OPTIONAL: -o [PATH_TO_SAVE_JPEG] (the path to output the resuling jpeg - defaults to 'img/out.jpg')
    OPTIONAL: -q [QUALITY] (the quality value to use for encoding - defaults to 80) 
    OPTIONAL: -s [DISPLAY_SCALE] (a multiplier which controls the size of the display window - defaults to 1)
```
Sample compilation command:
```
g++ examples\encode-decode.cpp common\*.cpp jpeg\src\*.cpp -o "jpeg.exe" -W -Wall -Wextra -pedantic -I "C:\SDL-release-2.26.4\include" -I "jpeg\inc" -I "common" -I "C:\w64devkit\include" "SDL2.dll" -std=c++20  -O3 -DNDEBUG
```

### Web-App
For fun, I have also compiled this library to WASM using Emscripten ([repeated link to web-app](http://www.wjgrace.co.uk/projects/jpeg/jpeg.html)). There is a basic HTML GUI which enables you to select from a few sample images to play around with. There is also the facility to upload bitmaps and download the resulting JPEGs .

Sample compilation command:
```
emcc examples/web-app.cpp common/*.cpp jpeg/src/*.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic -sUSE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" -I "jpeg\inc" -I "common" --preload-file img-cc/ -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=[_main,_malloc,_free] -std=c++20 -O3 -DNDEBUG
```

