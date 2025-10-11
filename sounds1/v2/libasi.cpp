/*
Notes for a dumb bunny:
- STOP USING std::vector::reserve()!!! It causes bad things when you then call .data()!
  You think you're so smart by not writing a couple zeroes (0.02 ms), but then you're gonna mess it up!

- fftw_malloc for fftw plans. Nothing else!

- FFTW_FORWARD: n -> (n/2 + 1)
- FFTW_REVERSE: (n/2 + 1) -> n

- Remember to normalize the output. How does it work? idfk

- When serializing floats you can use hexadecimal notation with libc's %a (e.g. 0xA.AB31p-1)

- DUDE! The default constructor for structs deep copies vectors. Bro is copying 100MB (why is it 100MB?) for no reason!!
  Just spam const references tbh.

- TODO: fix every single confusion of N/2 + 1 and N (I hate ts!)
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

// Completed
std::optional<Spectrodata> audio_to_spectro(const Audiodata& audio, samples_t window, samples_t step) 
{
    // 1. Prepare the result container.
    Spectrodata spectro;
    spectro.step = step;
    spectro.window = window;
    spectro.original_length = audio.data.size();
    spectro.sample_rate = audio.sample_rate;
    spectro.n_windows = 0;

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
        spectro.n_windows++;

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

std::optional<Audiodata> spectro_to_audio(const Spectrodata& in) {
    // Make no guarantee that the size of the spectrogram is divisible by its window size.

    // 1. Prepare the result container.
    Audiodata audio;
    std::cout << in.original_length;
    audio.data.resize(in.original_length);
    audio.sample_rate = in.sample_rate;

    samples_t output_window = in.window;
    samples_t input_window = output_window / 2 + 1;

    // 2. Allocate buffers for fftw to use.
    fftw_complex *input_buffer = fftw_alloc_complex(input_window);
    double *output_buffer = fftw_alloc_real(output_window);
    
    if (!input_buffer || !output_buffer) {
        std::cerr << "Failed to allocate memory for fourier transform!\n";
        fftw_free(input_buffer);
        fftw_free(output_buffer);
        return std::nullopt;
    }

    // 3. Create the plan.
    fftw_plan p = fftw_plan_dft_c2r_1d(output_window, input_buffer, output_buffer, FFTW_ESTIMATE);

    if (!p) {
        std::cerr << "Failed to create fftw plan for forward transform.\n";
        fftw_free(input_buffer);
        fftw_free(output_buffer);
        return std::nullopt;
    }

    // 4. This is an offset-add algorithm.

    // Iterate over all windows in the *input*.
    samples_t output_offset = 0;
    for (samples_t input_offset = 0; input_offset < in.data.size(); input_offset += input_window, output_offset += in.step) {
        // Copy that part of the spectrogram to the input buffer
        // Execute
        // Offset add, offset by the step size times i

        samples_t stop_point = input_window;

        if (input_offset + input_window > in.data.size()) {
            memset(input_buffer, 0, input_window * sizeof(fftw_complex));
            stop_point = in.data.size() - input_offset;
        }

        for (samples_t j = 0; j < stop_point; j++) {
            input_buffer[j][0] = in.data[input_offset + j].real();
            input_buffer[j][1] = in.data[input_offset + j].imag();
        }

        fftw_execute(p);

        stop_point = output_window;

        if (output_offset + output_window > audio.data.size()) {
            stop_point = audio.data.size() - output_offset;
        }

        for (samples_t j = 0; j < stop_point; j++) {
            audio.data[output_offset + j] += output_buffer[j] / output_window;
        }
    }
    
    fftw_destroy_plan(p);
    fftw_free(input_buffer);
    fftw_free(output_buffer);
    
    return audio;
}

// Completed
Imagedata_MONO spectro_to_image(const Spectrodata& in) {
    Imagedata_MONO out;
    out.data.resize(in.data.size());

    for (size_t i = 0; i < in.data.size(); i++) {
        auto re = in.data[i].real();
        auto im = in.data[i].imag();

        out.data[i] = magnitude_to_png(sqrt(re*re + im*im) / in.window, -60.0, -10.0);
    }

    out.height = in.window;
    out.width = in.n_windows;
    return out;
}

Spectrodata image_to_spectro(const Imagedata_MONO& in);

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
        
        sf_close(sndfile);
        return std::nullopt;
    }

    Audiodata audio{};
    audio.data.resize(sfinfo.frames);
    (void) sf_readf_double(sndfile, audio.data.data(), sfinfo.frames);
    audio.sample_rate = sfinfo.samplerate;

    sf_close(sndfile);
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

    sf_close(sndfile);
    return Audiodata_STEREO{left, right};
}

std::optional<Spectrodata> spectro_read(std::string fname);

int image_probe_channels(std::string fname);
std::optional<Imagedata_MONO> image_read_mono(std::string fname);
std::optional<Imagedata_RGBA> image_read_rgba(std::string fname);

// Write operations
int audio_write_mono(std::string fname, const Audiodata& audio) {
    SF_INFO sfinfo = {
        .samplerate=static_cast<int>(audio.sample_rate), 
        .channels=1, 
        .format=SF_FORMAT_WAV | SF_FORMAT_PCM_16, 
        .seekable=true
    };

    SNDFILE* sndfile = sf_open(fname.c_str(), SFM_WRITE, &sfinfo);

    if (!sndfile) {
        std::cerr << "audio_write_mono: failed to open audio file '" << fname << "'\n";
        return 0;
    }

    sf_count_t written = sf_writef_double(sndfile, audio.data.data(), audio.data.size());
    if (written < audio.data.size()) {
        std::cerr << "audio_write_mono: couldn't write all samples to output file '" << fname << "'\n";
        sf_close(sndfile);
        return 0;
    }

    sf_close(sndfile);
    return 1;
}

int audio_write_stereo(std::string fname, const Audiodata_STEREO& audio);
int audio_write_stereo(std::string fname, const Audiodata& left, const Audiodata& right);

int spectro_write(std::string fname, Spectrodata spectro);

// Completed
int image_write_mono(std::string fname, const Imagedata_MONO& image) {
    return stbi_write_png(fname.c_str(), image.width, image.height, 1, image.data.data(), image.width);
}

// Completed
int image_write_rgba(std::string fname, const Imagedata_RGBA& image) {
    return stbi_write_png(fname.c_str(), image.width, image.height, 4, image.data.data(), image.width * 4);
}

} // namespace asi