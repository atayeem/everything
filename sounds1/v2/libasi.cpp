/*
Notes for a dumb bunny:
- STOP USING std::vector::reserve()!!! It causes bad things when you then call .data()!
  You think you're so smart by not writing a couple zeroes (0.02 ms), but then you're gonna mess it up!

- fftw_malloc for fftw plans. Nothing else!

- FFTW_FORWARD: n -> (n/2 + 1)
- FFTW_REVERSE: (n/2 + 1) -> n

- Remember to normalize the output. How does it work? idfk

- When serializing floats you can use hexadecimal notation with libc's %a (e.g. 0xA.AB31p-1)
*/

#include "libasi.hpp"
#include <algorithm>
#include <iostream>

#include <sndfile.h>
#include <fftw3.h>

#include <string.h>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Completed
static double* get_hanning_window(samples_t window_size) {
    static std::unordered_map<samples_t, double*> windows;

    auto it = windows.find(window_size);

    if (it != windows.end()) {
        return it->second;
    } else {
        double *ptr = (double*) std::calloc(window_size, sizeof(double));
        if (!ptr)
            return NULL;

        for (int n = 0; n < window_size; n++)
            ptr[n] = 0.5 * (1 - cos(2.0 * M_PI * n / (window_size - 1)));

        windows[window_size] = ptr;
        return windows[window_size];
    }
}

// Completed
static uint8_t magnitude_to_png(double val, double min_db = -80.0, double max_db = 0.0) {
        double db = 20.0 * log10(val + 1e-6);

        // Clip to [min_db, max_db]
        db = std::clamp(db, min_db, max_db);

        // Normalize to 0..1
        double normalized = (db - min_db) / (max_db - min_db);

        // Convert to 0..255
        return static_cast<uint8_t>(normalized * 255.0 + 0.5);
}

// Converts `Audiodata` to `Spectrodata`.
// Fails on allocation failure, and on fftw failure.

namespace asi {

std::optional<Spectrodata> audio_to_spectro(Audiodata audio, samples_t window, samples_t step) 
{
    // 1. Prepare the result container.
    Spectrodata spectro;
    spectro.data.resize(window/2 + 1);
    spectro.step = step;
    spectro.window = window;

    // 2. Allocate buffers for fftw to use.
    double *input_buffer = static_cast<double*>(fftw_malloc(window * sizeof(double)));
    fftw_complex *output_buffer = static_cast<fftw_complex*>(fftw_malloc((window/2 + 1) * sizeof(fftw_complex)));

    if (!input_buffer || !output_buffer) {
        std::cerr << "Failed to allocate memory for fourier transform!\n";
        fftw_free(input_buffer);
        fftw_free(output_buffer);
        return std::nullopt;
    }

    // 3. Create the plan.
    fftw_plan p = fftw_plan_dft_r2c_1d(window, input_buffer, output_buffer, FFTW_ESTIMATE);

    if (!p) {
        std::cerr << "Failed to create fftw plan for forward transform.\n";
        fftw_free(input_buffer);
        fftw_free(output_buffer);
        return std::nullopt;
    }

    samples_t offset = 0;
    bool running = true;
    auto han = get_hanning_window(window);

    // 4. Iterate over all the data. If a length of `window` past the offset would overshoot, truncate the window,
    //    and pad the buffer with zeroes. Window the data. Execute. Keep copying the data to the back of the output.
    while (running) {
        if (offset + window > audio.data.size()) {
            running = false;
            samples_t tail_size = audio.data.size() - offset;

            memset(input_buffer, 0, sizeof(double) * window);
            memcpy(input_buffer, audio.data.data() + offset, sizeof(double) * tail_size);
        } else {
            memcpy(input_buffer, audio.data.data() + offset, sizeof(double) * window);
        }

        for (samples_t i = 0; i < window; i++) {
            input_buffer[i] = input_buffer[i] * han[i];
        }

        fftw_execute(p);

        for (int i = 0; i < window/2 + 1; ++i) {
            spectro.data.emplace_back(output_buffer[i][0], output_buffer[i][1]); // real, imag
        }

        offset += step;
    }

    fftw_destroy_plan(p);
    fftw_free(input_buffer);
    fftw_free(output_buffer);
    
    return spectro;
}

Audiodata spectro_to_audio(Spectrodata in, samples_t sample_rate);

Imagedata_MONO spectro_to_image(Spectrodata in) {
    Imagedata_MONO out;
    out.data.resize(in.data.size());

    for (size_t i = 0; i < in.data.size(); i++) {
        auto re = in.data[i].real();
        auto im = in.data[i].imag();

        out.data[i] = magnitude_to_png(sqrt(re*re + im*im) / in.window, -60.0, -10.0);
    }

    out.height = in.window;
    out.width = in.data.size() / in.window;
    return out;
}

Spectrodata image_to_spectro(Imagedata_MONO in);

// Completed
int audio_probe_channels(std::string fname)
{
    SF_INFO sfinfo{};

    SNDFILE* sndfile = sf_open(fname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening audio file '" << fname << "': " 
                  << sf_strerror(nullptr) << "\n";
        return 0;
    }

    int channels = sfinfo.channels;
    sf_close(sndfile);
    return channels;
}

// Completed
std::optional<Audiodata> audio_read_mono(std::string fname)
{
    SF_INFO sfinfo{};

    SNDFILE *sndfile = sf_open(fname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening audio file '" << fname << "': " 
                  << sf_strerror(nullptr) << "\n";
        
        return std::nullopt;
    }

    if (sfinfo.channels != 1) {
        std::cerr << fname << " is not a mono audio file.\n";
        return std::nullopt;
    }

    Audiodata audio{};
    audio.data.resize(sfinfo.frames);
    (void) sf_readf_double(sndfile, audio.data.data(), sfinfo.frames);
    audio.sample_rate = sfinfo.samplerate;

    return audio;
}

// Completed
std::optional<Audiodata_STEREO> audio_read_stereo(std::string fname)
{
    SF_INFO sfinfo{};

    SNDFILE *sndfile = sf_open(fname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening audio file '" << fname << "': " 
                  << sf_strerror(nullptr) << "\n";
        
        return std::nullopt;
    }

    if (sfinfo.channels != 2) {
        std::cerr << fname << " is not a stereo audio file.\n";
        return std::nullopt;
    }

    std::vector<double> buffer(sfinfo.frames * 2);

    (void) sf_readf_double(sndfile, buffer.data(), sfinfo.frames);

    Audiodata left{};
    Audiodata right{};

    left.data.resize(sfinfo.frames);
    right.data.resize(sfinfo.frames);

    left.sample_rate = sfinfo.samplerate;
    right.sample_rate = sfinfo.samplerate;

    for (long i = 0; i < sfinfo.frames; ++i) {
        left.data[i] = buffer[2*i];
        right.data[i] = buffer[2*i + 1];
    }

    return Audiodata_STEREO{left, right};
}

std::optional<Spectrodata> spectro_read(std::string fname);

int image_probe_channels(std::string fname);
std::optional<Imagedata_MONO> image_read_mono(std::string fname);
std::optional<Imagedata_RGBA> image_read_rgba(std::string fname);

// Write operations
int audio_write_mono(std::string fname, Audiodata audio);
int audio_write_stereo(std::string fname, Audiodata_STEREO audio);
int audio_write_stereo(std::string fname, Audiodata left, Audiodata right);

int spectro_write(std::string fname, Spectrodata spectro);

// Completed
int image_write_mono(std::string fname, Imagedata_MONO image) {
    std::cout << "stbi_write_png(" << image.width << ", " << image.height << ", " << 1 << ", " << (void*) image.data.data() << ", " << image.width << ");" << std::endl;
    return stbi_write_png(fname.c_str(), image.width, image.height, 1, image.data.data(), image.width);
}

// Completed
int image_write_rgba(std::string fname, Imagedata_RGBA image) {
    return stbi_write_png(fname.c_str(), image.width, image.height, 4, image.data.data(), image.width * 4);
}

} // namespace asi