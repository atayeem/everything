#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <fftw3.h>

#define RE 0
#define IM 1

#define SAMPLE_RATE_HZ 44100
#define SZ 2048

static double w[SZ];
static double *transform_in;
static fftw_complex *transform_out;
static fftw_plan p;

int get_spec_init() 
{
    // Generate window function
    for (int n = 0; n < SZ; n++)
        w[n] = 0.5 * (1 - cos(2.0 * M_PI * n / (SZ - 1)));
    
    transform_in = fftw_malloc(sizeof(double) * SZ);
    transform_out = fftw_malloc(sizeof(fftw_complex) * SZ);

    if (!transform_in || !transform_out)
        return 0;

    p = fftw_plan_dft_r2c_1d(SZ, transform_in, transform_out, FFTW_ESTIMATE);

    if (!p)
        return 0;

    return 1;
}

int get_spec(const double *in, double *out)
{
    double *final;

    for (int i = 0; i < SZ; i++)
        transform_in[i] = in[i] * w[i];

    fftw_execute(p);

    memcpy(out, transform_out, sizeof(double) * SZ);
}

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Streaming Textures", "1.0", "com.example.renderer-streaming-textures");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/streaming-textures", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (!texture) {
        SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    get_spec_init();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 now = SDL_GetTicks();
    SDL_Surface *surface = NULL;

    if (SDL_LockTextureToSurface(texture, NULL, &surface)) {
        SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(surface->format), NULL, 0, 0, 0));

        for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
            ((unsigned int*)surface->pixels)[i] = 0xFF0000FF;
        }

        SDL_UnlockTexture(texture);
    }

    SDL_RenderClear(renderer);

    SDL_FRect dst_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.w = WINDOW_WIDTH;
    dst_rect.h = WINDOW_HEIGHT;

    SDL_RenderTexture(renderer, texture, NULL, &dst_rect);

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
}
