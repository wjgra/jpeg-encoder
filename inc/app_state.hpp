#ifndef _FLUID_APP_STATE_HPP_
#define _FLUID_APP_STATE_HPP_

#include <iostream>
#include <chrono>

#include "../inc/window.hpp"

class AppState{
public:
    AppState(unsigned int scale = 1);
    void beginLoop();
    void mainLoop();
    void handleEvents(SDL_Event const&  event);
    void frame(unsigned int frameTime);
    void quitApp();
    // Window/context state parameters
    bool quit = false;
    SDL_Event event;
    std::chrono::time_point<std::chrono::high_resolution_clock> tStart, tNow;
    // -- Viewport resolution is winScale * notional dimension
    unsigned int const winScale = 1;
    // -- Dimensions of notional window
    int const winWidth = 640;
    int const winHeight = 480;
   Window window;

};
#endif