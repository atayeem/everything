#include "libasi.hpp"
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <iostream>

#include <sndfile.h>
#include <fftw3.h>

#include <string.h>
#include <unordered_map>

#define STBI_FAILURE_USERMSG
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

// Completed
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

// Spectrograms are column-major while images are row-major.
// This needs to fix that.
Imagedata_MONO spectro_to_image(const Spectrodata& in) {
    Imagedata_MONO out;
    // n = 1000
    // 1000 duration x 501 bins = 501000
    out.data.resize(in.data.size());

    auto bin_size = in.window / 2 + 1;
    auto n_bins = in.n_windows;

    assert(bin_size * n_bins == in.data.size());

    for (size_t row = 0; row < n_bins; row++) {
        for (size_t col = 0; col < bin_size; col++) {
            auto re = in.data[row*bin_size + col].real();
            auto im = in.data[row*bin_size + col].imag();

            out.data[row + col*n_bins] = magnitude_to_png(sqrt(re*re + im*im) / in.window, -60.0, 0.0);
        }
    }

    out.height = in.window / 2 + 1;
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

// Completed
int image_probe_channels(std::string fname) {
    int x, y, comp;

    if (!stbi_info(fname.c_str(), &x, &y, &comp)) {
        std::cerr << "image_probe_channels: failed to get information on image '" << fname << "'\n";
        return 0;

    } else {
        return comp;
    }
}

// Completed
std::optional<Imagedata_MONO> image_read_mono(std::string fname) {
    if (image_probe_channels(fname) != 1) {
        std::cerr << "image_read_mono: image '" << fname << "' does not have 1 channel.\n";
        return std::nullopt;
    }

    int x, y, comp;
    unsigned char *data = stbi_load(fname.c_str(), &x, &y, &comp, 0);

    if (!data) {
        std::cerr << "image_read_mono: failed to read image '" << fname << "': " << stbi_failure_reason() << "\n";
        return std::nullopt;
    }

    Imagedata_MONO out;
    out.width = x;
    out.height = y;
    out.data.resize(x * y);

    std::copy_n(data, x * y, out.data.begin());

    stbi_image_free(data);
    return out;
}

// Completed
std::optional<Imagedata_MONOA> image_read_monoa(std::string fname) {
    if (image_probe_channels(fname) != 2) {
        std::cerr << "image_read_monoa: image '" << fname << "' does not have 2 channels.\n";
        return std::nullopt;
    }

    int x, y, comp;
    unsigned char *data = stbi_load(fname.c_str(), &x, &y, &comp, 0);

    if (!data) {
        std::cerr << "image_read_monoa: failed to read image '" << fname << "': " << stbi_failure_reason() << "\n";
        return std::nullopt;
    }

    Imagedata_MONOA out;

    out.v.width = out.a.width = x;
    out.v.height = out.a.height = y;

    for (size_t i = 0; i < x * y; i++) {
        out.v.data[i] = data[2*i];
        out.a.data[i] = data[2*i + 1];
    }

    stbi_image_free(data);
    return out;
}

// Completed
std::optional<Imagedata_RGB> image_read_rgb(std::string fname) {
    if (image_probe_channels(fname) != 3) {
        std::cerr << "image_read_rgb: image '" << fname << "' does not have 3 channels.\n";
        return std::nullopt;
    }

    int x, y, comp;
    unsigned char *data = stbi_load(fname.c_str(), &x, &y, &comp, 0);

    if (!data) {
        std::cerr << "image_read_rgb: failed to read image '" << fname << "': " << stbi_failure_reason() << "\n";
        return std::nullopt;
    }

    Imagedata_RGB out;

    out.r.width = out.g.width = out.b.width = x;
    out.r.height = out.g.height = out.b.height = y;

    for (size_t i = 0; i < x * y; i++) {
        out.r.data[i] = data[3*i];
        out.g.data[i] = data[3*i + 1];
        out.b.data[i] = data[3*i + 2];
    }

    stbi_image_free(data);
    return out;
}

// Completed
std::optional<Imagedata_RGBA> image_read_rgba(std::string fname) {
    if (image_probe_channels(fname) != 4) {
        std::cerr << "image_read_rgba: image '" << fname << "' does not have 4 channels.\n";
        return std::nullopt;
    }

    int x, y, comp;
    unsigned char *data = stbi_load(fname.c_str(), &x, &y, &comp, 0);

    if (!data) {
        std::cerr << "image_read_rgba: failed to read image '" << fname << "': " << stbi_failure_reason() << "\n";
        return std::nullopt;
    }

    Imagedata_RGBA out;

    out.r.width = out.g.width = out.b.width = out.a.width = x;
    out.r.height = out.g.height = out.b.height = out.a.height = y;

    for (size_t i = 0; i < x * y; i++) {
        out.r.data[i] = data[4*i];
        out.g.data[i] = data[4*i + 1];
        out.b.data[i] = data[4*i + 2];
        out.a.data[i] = data[4*i + 3];
    }

    stbi_image_free(data);
    return out;
}

// Completed
int audio_write_mono(std::string fname, const Audiodata& audio) {
    SF_INFO sfinfo = {
        .samplerate=static_cast<int>(audio.sample_rate), 
        .channels=1, 
        .format=SF_FORMAT_WAV | SF_FORMAT_PCM_16, 
        .seekable=true
    };

    SNDFILE* sndfile = sf_open(fname.c_str(), SFM_WRITE, &sfinfo);

    if (!sndfile) {
        std::cerr << "audio_write_mono: failed to open audio file '" << fname << "': " << sf_strerror(sndfile) << "\n";
        return 0;
    }

    sf_count_t written = sf_writef_double(sndfile, audio.data.data(), audio.data.size());
    if (written < audio.data.size()) {
        std::cerr << "audio_write_mono: couldn't write all samples to output file '" << fname << "': " << sf_strerror(sndfile) << "\n";
        sf_close(sndfile);
        return 0;
    }

    sf_close(sndfile);
    return 1;
}

// Completed
int audio_write_stereo(std::string fname, const Audiodata_STEREO& audio) {
    return audio_write_stereo(fname, audio.left, audio.right);
}

// Completed
int audio_write_stereo(std::string fname, const Audiodata& left, const Audiodata& right) {

    if (left.sample_rate != right.sample_rate) {
        std::cerr << "audio_write_stereo: left and right channels have different sample rates. This is unsupported.\n";
        return 0;
    }

    SF_INFO sfinfo = {
        .samplerate=static_cast<int>(left.sample_rate), 
        .channels=2, 
        .format=SF_FORMAT_WAV | SF_FORMAT_PCM_16, 
        .seekable=true
    };

    SNDFILE* sndfile = sf_open(fname.c_str(), SFM_WRITE, &sfinfo);

    if (!sndfile) {
        std::cerr << "audio_write_mono: failed to open audio file '" << fname << "'\n";
        return 0;
    }

    // If one audio is short, the short one will be padded with zeroes.

    samples_t left_size  = left.data.size();
    samples_t right_size = right.data.size();

    samples_t long_size = std::max(left_size, right_size);

    std::vector<double> data_to_write(2 * long_size);

    for (samples_t i = 0; i < long_size; i++) {
        data_to_write[2*i]     = (i < left_size)  ? left.data[i]  : 0.0;
        data_to_write[2*i + 1] = (i < right_size) ? right.data[i] : 0.0;
    }

    sf_count_t written = sf_writef_double(sndfile, data_to_write.data(), long_size);

    if (written < long_size) {
        std::cerr << "audio_write_stereo: couldn't write all samples to output file '" << fname << "'\n";
        sf_close(sndfile);
        return 0;
    }

    sf_close(sndfile);
    return 1;
}

int spectro_write(std::string fname, Spectrodata spectro);

// Completed
int image_write_mono(std::string fname, const Imagedata_MONO& image) {
    stbi_flip_vertically_on_write(true);
    return stbi_write_png(fname.c_str(), image.width, image.height, 1, image.data.data(), image.width);
}

// Completed
int image_write_monoa(std::string fname, const Imagedata_MONOA& image) {
    auto value = image.v;
    auto alpha = image.a;

    std::vector<uint8_t> data_to_write(value.height * value.width * 2);

    for (size_t i = 0; i < value.height * value.width; i++) {
        data_to_write[2*i] = value.data[i];
        data_to_write[2*i + 1] = alpha.data[i];
    }

    stbi_flip_vertically_on_write(true);
    return stbi_write_png(fname.c_str(), value.width, value.height, 2, data_to_write.data(), 2 * value.width);
}

// Completed
int image_write_rgb(std::string fname, const Imagedata_RGB& image) {
    auto r = image.r;
    auto g = image.g;
    auto b = image.b;

    std::vector<uint8_t> data_to_write(r.height * r.width * 3);

    for (size_t i = 0; i < r.height * r.width; i++) {
        data_to_write[3*i] = r.data[i];
        data_to_write[3*i + 1] = g.data[i];
        data_to_write[3*i + 2] = b.data[i];
    }

    stbi_flip_vertically_on_write(true);
    return stbi_write_png(fname.c_str(), r.width, r.height, 3, data_to_write.data(), 3 * r.width);
}

// Completed
int image_write_rgba(std::string fname, const Imagedata_RGBA& image) {
    auto r = image.r;
    auto g = image.g;
    auto b = image.b;
    auto a = image.a;

    std::vector<uint8_t> data_to_write(r.height * r.width * 4);

    for (size_t i = 0; i < r.height * r.width; i++) {
        data_to_write[4*i] = r.data[i];
        data_to_write[4*i + 1] = g.data[i];
        data_to_write[4*i + 2] = b.data[i];
        data_to_write[4*i + 3] = b.data[i];
    }
    stbi_flip_vertically_on_write(true);
    return stbi_write_png(fname.c_str(), r.width, r.height, 4, data_to_write.data(), 4 * r.width);
}
#define LIBASI_USE_SDL
#ifdef LIBASI_USE_SDL

#endif

} // namespace asi