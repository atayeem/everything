#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_main.h>

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) == false) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello SDL3",
                                       800, 600,
                                       SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bool running = true;

    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Do your rendering or game logic here
        SDL_Delay(16);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}