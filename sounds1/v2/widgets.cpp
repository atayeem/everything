#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_properties.h>
#include <vector>
#include <string>

namespace Wd {

class Object {

};

class Window {
    Wd::Window &parent;
    std::vector<Window*> children;

    SDL_Window *ptr;
    SDL_Renderer *renderer;

    Window(const std::string& title, SDL_WindowFlags flags=0, int w=0, int h=0) {
        SDL_CreateWindowAndRenderer(title.c_str(), w, h, flags, &ptr, &renderer);
    }
};

}