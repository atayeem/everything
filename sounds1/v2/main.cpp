#include <iostream>
#include <vector>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_thread.h>

#include <sndfile.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

int play_music(void *fname_charptr) {
    const char *fname = static_cast<const char*>(fname_charptr);

    SNDFILE *file;
    SF_INFO info;
    
    file = sf_open(fname, SFM_READ, &info);
    if (!file) {
        std::cerr << "Error opening file: " << sf_strerror(NULL) << "\n";
        return 0;
    }

    std::vector<float> buffer(info.frames * info.channels);

    sf_count_t read = sf_readf_float(file, buffer.data(), info.frames);

    sf_close(file);

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_F32;
    spec.channels = info.channels;
    spec.freq = info.samplerate;

    SDL_AudioStream *as = SDL_CreateAudioStream(&spec, &spec);

    if (!as) {
        std::cerr << "Error creating audio stream: " << SDL_GetError() << "\n";
        return 0;
    }

    if (!SDL_PutAudioStreamData(as, buffer.data(), buffer.size() * sizeof(float))) {
        std::cerr << "PutAudioStreamData failed: " << SDL_GetError() << "\n";
        return 0;
    }

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!dev) {
        std::cerr << "OpenAudioDevice failed: " << SDL_GetError() << "\n";
        return 0;
    }

    SDL_BindAudioStream(dev, as);

    while (SDL_GetAudioStreamQueued(as) > 0) {
        SDL_Delay(200);
    }

    SDL_CloseAudioDevice(dev);
    SDL_DestroyAudioStream(as);

    return 1;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Sounds1 Project", "0.1.0", "io.github.atayeem.sounds1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("sounds1", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Thread *thread = SDL_CreateThread(play_music, "play_music thread", (void*)"test.wav");

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
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
