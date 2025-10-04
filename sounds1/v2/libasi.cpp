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
#include <iostream>

#include <sndfile.h>
#include <fftw3.h>
#include <string.h>

using namespace asi;

// Converts `Audiodata` to `Spectrodata`.
// Fails on allocation failure, and on fftw failure.
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

    // 4. Iterate over all the data. If a length of `window` past the offset would overshoot, truncate the window,
    //    and pad the buffer with zeroes. Keep copying the data to the back of the output.
    while (running) {
        if (offset + window > audio.data.size()) {
            running = false;
            samples_t tail_size = audio.data.size() - offset;

            memset(input_buffer, 0, sizeof(double) * window);
            memcpy(input_buffer, audio.data.data() + offset, sizeof(double) * tail_size);
        } else {
            memcpy(input_buffer, audio.data.data() + offset, sizeof(double) * window);
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
Imagedata_MONO spectro_to_image(Spectrodata in);
Spectrodata image_to_spectro(Imagedata_MONO in);

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

int image_write_mono(std::string fname, Imagedata_MONO image);
int image_write_rgba(std::string fname, Imagedata_RGBA image);