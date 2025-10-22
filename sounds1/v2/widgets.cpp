#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_properties.h>
#include <vector>

namespace Wd {

class Element {
`
};

class Widget {

};

class Window {
    Wd::Window *parent;
    SDL_Window *ptr;
    std::vector<Wd::Widget*> widgets;
    Window() {

    }
};

}