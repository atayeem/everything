#include <SDL3/SDL_pixels.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_properties.h>

#include "widgets.cpp"
#include <vector>

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Sounds1 Project", "0.1.0", "io.github.atayeem.sounds1");

    return SDL_APP_CONTINUE;
}

/* This loop is only meant to handle events that aren't user input. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

static void handle_event(SDL_Event *e) {
    switch(e->type) {
        case SDL_EVENT_KEY_DOWN:
            break;
    }
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_Event e;
    if (!SDL_WaitEvent(&e)) {
        SDL_Log("Failed to get event: %s", SDL_GetError());
        return SDL_APP_CONTINUE;
    }

    handle_event(&e);
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
