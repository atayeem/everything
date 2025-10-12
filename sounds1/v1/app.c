#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_audio.h>

#include <fftw3.h>
#include <sndfile.h>

#define SAMPLE_RATE_HZ 44100
#define SZ 2048

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static SNDFILE *audio_file;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_AudioStream *audiostream = NULL;

// This function nicely initializes itself for you.
// It returns 0 on failure, and 1 on success.
// Failure can only occur on the first call to the function.
static int get_spec(const double *in, double *out)
{
    static bool initialized = false;
    static double w[SZ];
    static double *transform_in;
    static fftw_complex *transform_out;
    static fftw_plan p;

    if (!initialized) {
        // Generate window function
        for (int n = 0; n < SZ; n++)
            w[n] = 0.5 * (1 - cos(2.0 * M_PI * n / (SZ - 1)));
        
        transform_in = fftw_malloc(sizeof(double) * SZ);
        if (!transform_in) {
            return 0;
        }

        transform_out = fftw_malloc(sizeof(fftw_complex) * (SZ/2 + 1));
        if (!transform_out) {
            fftw_free(transform_in);
            return 0;
        }

        p = fftw_plan_dft_r2c_1d(SZ, transform_in, transform_out, FFTW_ESTIMATE);

        if (!p) {
            fftw_free(transform_in);
            fftw_free(transform_out);
            return 0;
        }

        initialized = true;
    }

    // Window and copy input to fftw memory
    for (int i = 0; i < SZ; i++)
        transform_in[i] = in[i] * w[i];

    // Execute from N real numbers to N/2 + 1 complex numbers
    fftw_execute(p);
    
    // For each complex number, just compute the magnitude (we don't need phase right now)
    for (int k = 0; k < (SZ / 2 + 1); k++) {
        double re = transform_out[k][0];
        double im = transform_out[k][1];
        out[k] = sqrt(re*re + im*im);
    }

    return 1;
}



SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Sounds1 Project", "0.1", "io.github.atayeem.sounds1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("sounds1", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (!texture) {
        SDL_Log("Couldn't create streaming texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SF_INFO sfinfo  = {
        .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    };

    char *filename = "test.wav";

    audio_file = sf_open(filename, SFM_READ, &sfinfo);
    if (!audio_file) {
        SDL_Log("Failed to open the wav file %s.\n", filename);
        return SDL_APP_FAILURE;
    }

    SDL_AudioSpec audiospec = {SDL_AUDIO_S16, 2, SAMPLE_RATE_HZ};

    audiostream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audiospec, NULL, NULL);

    if (!audiostream) {
        SDL_Log("Couldn't create the AudioStream (whatever that is): %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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

        for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i += WINDOW_WIDTH) {
            ((unsigned int*)surface->pixels)[i] = 0xFF0000FF;
        }

        SDL_UnlockTexture(texture);
    }

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
}
