#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_properties.h>
#include <vector>
#include <utility>
#include <string>

namespace Wd {

struct Element {
    SDL_Texture *tex;
    int x, y, w, h;
    bool is_focused;
    bool is_focusable;

    struct Element *parent;

    bool (*event)(struct Element *self, const SDL_Event *e);
    bool (*unfocus)(struct Element *self);
    bool (*update)(struct Element *self, double dt);
    bool (*redraw)(struct Element *self, int w, int h, bool is_focused, SDL_Texture **tex);
    bool (*render)(struct Element *self, SDL_Renderer *r);
    void (*destroy)(struct Element *self);
};

}