Compiles with no warnings with:
'''
g++ src\*.cpp -o "jpeg.exe" -W -Wall -Wextra -pedantic -I "C:\SDL-release-2.26.4\include" "SDL2.dll"  -std=c++20 -I "C:\w64devkit\include" 
'''
For Emscripten (strangely wildcard expansion seems only to work in bash, not Powershell):
```
emcc src/*.cpp -o "jpeg.html" -W -Wall -Wextra -pedantic -std=c++20 -s USE_SDL=2 --shell-file template.html -I "C:\Users\wjgra\source\repos\emsdk\upstream\emscripten\cache\sysroot\include" -fexceptions
```