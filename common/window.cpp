#include "window.hpp"

Window::Window(unsigned int width, unsigned int height, std::string const& title) : winWidth{width}, winHeight{height}{
    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        throw std::string("Failed to initialise SDL");
    }
    // Create window
    window = SDL_CreateWindow(
        title.c_str(), 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        winWidth, 
        winHeight, 
        winFlags);

    if (!window){
        throw std::string("Failed to create window");
    }
}

#include <iostream>
Window::~Window(){
    SDL_DestroyWindow(window);
    SDL_Quit();
}

SDL_Window* Window::getWindow(){
    return window;
}

void Window::toggleFullScreen(){
    fullScreen = !fullScreen;
    if (fullScreen){
        SDL_SetWindowFullscreen(window, winFlags | SDL_WINDOW_FULLSCREEN);
    }
    else{
        SDL_SetWindowFullscreen(window, winFlags);
    }
}

void Window::resize(unsigned int width, unsigned int height){
    winWidth = width;
    winHeight = height;
    SDL_SetWindowSize(window, width, height);
}

void Window::setTitle(std::string const& title){
     SDL_SetWindowTitle(window, title.c_str());
}