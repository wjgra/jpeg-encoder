Compiles with no warnings with:
'''
g++ src\*.cpp -o "jpeg.exe" -W -Wall -Wextra -pedantic -g -I "C:\SDL-release-2.26.4\include" "SDL2.dll"  -std=c++20
'''
For some reason wildcards don't work with emcc in Powershell, but this also compiles with:
```
emcc src\main.cpp src\app_state.cpp src\stb_image.cpp src\window.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic -std=c++20 -s USE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" -fexceptions
```