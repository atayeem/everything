#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_properties.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include <stdexcept>
#include <vector>

namespace Wd {

typedef struct Element_CB Element_CB;
typedef struct Element Element;
typedef struct Window Window;

struct Element {
    SDL_Texture *tex;
    SDL_Renderer *renderer;
    Window *parent_window;

    int x, y, w, h;

    bool is_focused;

    Element(Window *parent_window, SDL_Renderer *renderer, int x=0, int y=0, int w=100, int h=100)
        : parent_window(parent_window), renderer(renderer), x(x),  y(y),  w(w),  h(h), is_focused(false) 
    {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
        
        if (!tex) {
            throw std::runtime_error(SDL_GetError());
        }

        change_color(0xFF0000FF);
    }

    virtual ~Element() 
    {
        SDL_DestroyTexture(tex);
    }

    virtual void event(const SDL_Event *e) {
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            change_color(0xFF00FF00);
        }
        if (e->type == SDL_EVENT_MOUSE_BUTTON_UP) {
            change_color(0xFF0000FF);
        }
    }

    private:
    void change_color(uint32_t color) {
        void *pixels_vp;
        int pitch;
        
        if (SDL_LockTexture(tex, nullptr, &pixels_vp, &pitch)) {
        
            uint32_t *pixels = static_cast<uint32_t*>(pixels_vp);
            for (long i = 0; i < w * h; i++) {
                pixels[i] = color;
            }

            SDL_UnlockTexture(tex);
        }
    }
};

struct GridElement {
    int row, col, row_span, col_span;
    Element *element;
};

struct Window {
    std::vector<Element*> elements;
    int grid_w, grid_h;
};

}

#define STANDALONE_TEST
#ifdef STANDALONE_TEST

static SDL_Renderer *renderer;
static SDL_Window *window;

static Wd::Element *test_element;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Sounds1 Project", "0.1.0", "io.github.atayeem.sounds1");
    SDL_CreateWindowAndRenderer("sounds1", 640, 480, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE, &window, &renderer);
    test_element = new Wd::Element(nullptr, renderer, 0, 0, 640, 480);
    return SDL_APP_CONTINUE;
}

/* This loop is only meant to handle events that aren't user input. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e)
{
    switch (e->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        default:
            test_element->event(e);
            return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_FRect dstrect = {0, 0, 640, 480};

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, test_element->tex, nullptr, &dstrect);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    delete test_element;
}

#endif