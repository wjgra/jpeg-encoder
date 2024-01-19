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
    void resize(unsigned int width, unsigned int height);
    void setTitle(std::string const& title = "[SET TITLE]");
    unsigned int winWidth;
    unsigned int winHeight;
private:
    SDL_Window* window = nullptr;
    bool fullScreen = false;
    Uint32 winFlags = SDL_WINDOW_SHOWN;
};

#endif