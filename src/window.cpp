#include "../inc/window.hpp"

Window::Window(unsigned int width, unsigned int height, std::string title) : winWidth{width}, winHeight{height}, title{title}{
    // Initialise SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        throw std::string("Failed to initialise SDL");
    }

    #ifndef __EMSCRIPTEN__
    SDL_GL_LoadLibrary(nullptr); 
    #endif

    // Set OpenGL attributes - must be done before window creation!
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    #ifndef __EMSCRIPTEN__
    // Request an OpenGL 3.3 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    #endif
    
    // Request a depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // Request multisampling
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

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

void Window::frame(unsigned int frameTime){
    // Calculate average frame length and FPS for display in title bar
    accumulatedFrameTime += frameTime;
    ++numFrames;
    if (accumulatedFrameTime > 1000000){ // every second
        float avgFrameTime = (float)accumulatedFrameTime / ((float)numFrames*1000.0f); // in ms
        int FPS = int(1000.0f/avgFrameTime);
        SDL_SetWindowTitle(window, 
            std::string(title + " - FPS: "+std::to_string(FPS)+" ("+std::to_string(avgFrameTime)+" ms)").c_str());
        accumulatedFrameTime = 0;
        numFrames = 0;
    }
}