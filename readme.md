To compile native example:
'''
g++ examples\encode-decode.cpp common\*.cpp jpeg\src\*.cpp -o "jpeg.exe" -W -Wall -Wextra -pedantic -I "C:\SDL-release-2.26.4\include" "SDL2.dll"  -std=c++20 -I "C:\w64devki
t\include" -O3
'''
To compile web-app:
```
emcc examples/web-app.cpp common/*.cpp jpeg/src/*.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic -std=c++20 -sUSE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" --preload-file img-cc/ -sALLOW_MEMORY_GROWTH=1 -O3 -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=[_main,_malloc,_free]
```