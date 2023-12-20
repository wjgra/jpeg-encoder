#ifndef _FLUID_WINDOW_HPP_
#define _FLUID_WINDOW_HPP_

#include <SDL.h>
#include <string>

class Window{
public:
    Window(unsigned int width, unsigned int height, std::string const& title = "[SET TITLE]");
    ~Window();
    Window(const Window& other) = delete;
    Window &operator=(const Window& other) = delete;
    Window(Window&& other) = delete;
    Window &operator=(Window&& other) = delete;
    SDL_Window* getWindow();
    void toggleFullScreen();
    unsigned int const winWidth;
    unsigned int const winHeight;
private:
    SDL_Window* window = nullptr;
    bool fullScreen = false;
    Uint32 winFlags = SDL_WINDOW_SHOWN;
};

#endif