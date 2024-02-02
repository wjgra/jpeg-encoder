# JPEG Encoding and Decoding Library
## Overview
This is a collection of C++ classes that enable encoding of 24-bit bitmap images to baseline seqeuntial JPEGs as specified in [ITU T.81](https://www.w3.org/Graphics/JPEG/itu-t81.pdf) (link to PDF) and decoding such JPEGs to recover an approximation of the original bitmap. Please do not rely on this for anything important, it was primarily a fun learning exercise, and my first foray into the world of image compression. I prioritised 1) fun and 2) trying out C++17/20 features that I hadn't used before over speed, but it's fast enough to use interactively ([link to web-app](http://www.wjgrace.co.uk/projects/jpeg.html)).

There are lots of extensions that I could add at some point in the future (e.g. progressive encoding, arithmetic encoding), but this seems feature-complete enough for now I'm happy to leave it for the near-future, critical bugs notwithstanding.

## Dependencies
Bitmap loading is handled by SDL's loadImage function, as I was already using it in the two example to handle window creation and display of the results. Writing my own bitmap parser would be a relatively simple task, but I may be better off using one of the many header-only bitmap loaders.

Otherwise, the JPEG encoder-decoder classes in this repository only depend on the C++20 STL.

## Compilation of Example Programs
Both examples compile on my computer with no warnings. Some sample g++ commands are below, though obviously these may differ based on your environment.

### Native Example
Takes a single 24-bit RGB bitmap as input, encodes it as a JPEG, then decodes the compressed JPEG version for display side-by-side with the original. For small images, the scale parameter may be used to 'blow them up' such that they are more clearly visible. 

Usage (parameters may be provided in any order):
```
.\jpeg.exe -i [PATH_TO_INPUT_BMP] -o [OPTIONAL: PATH_TO_SAVE_JPEG] -q [OPTIONAL: QUALITY (default = 80)] -s [OPTIONAL DISPLAY_SCALE (default = 1)]
```
Sample compilation command:
```
g++ examples\encode-decode.cpp common\*.cpp jpeg\src\*.cpp -o "jpeg.exe" -W -Wall -Wextra -pedantic -I "C:\SDL-release-2.26.4\include" -I "jpeg\inc" -I "common" -I "C:\w64devkit\include" "SDL2.dll" -std=c++20  -O3 -DNDEBUG
```

### Web-App
For fun, I have also compiled this library to WASM using Emscripten ([repeated link to web-app](http://www.wjgrace.co.uk/projects/jpeg.html)). There is a basic HTML GUI which enables you to select from a few sample images to play around with. There is also the facility to upload bitmaps and download the resulting JPEGs .

Sample compilation command:
```
emcc examples/web-app.cpp common/*.cpp jpeg/src/*.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic -sUSE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" -I "jpeg\inc" -I "common" --preload-file img-cc/ -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=[_main,_malloc,_free] -std=c++20 -O3 -DNDEBUG
```

