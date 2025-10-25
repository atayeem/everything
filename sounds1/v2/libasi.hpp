#pragma once

#include <complex>
#include <cstdint>
#include <optional>
#include <vector>
#include <cstdlib>

namespace asi {

using samples_t = size_t;

enum class Ordering {
    /*
    Imagedata: The image is a sequence of rows left to right, arranged in order from top to bottom.
    Spectrodata: The fft bins are interlaced, rather than arranged in order of time.
    */
    ROW_MAJOR,

    /*
    Imagedata: The image is a sequence of columns top to bottom, arranged in order from left to right.
    Spectrodata: The fft bins are arranged by timestep.
    */
    COLUMN_MAJOR,

    AUDIO_SEQUENTIAL = ROW_MAJOR,
    INTERLACED = COLUMN_MAJOR,
};
    
struct Audiodata {
    std::vector<double> data;
    samples_t sample_rate;

    int channels;
    Ordering ordering;
};

struct Imagedata {
    std::vector<uint8_t> data;
    size_t width;
    size_t height;
};

struct Spectrodata {
    std::vector<std::complex<double>> data;
    samples_t window;
    samples_t step;

    // Used to make Audiodata -> Spectrodata reversible.
    samples_t sample_rate;
    samples_t original_length;

    // Used to facilitate Spectrodata -> Imagedata_MONO
    size_t n_windows;
};

// Conversion of the 3 basic types
// Imagedata_MONO <=> Spectrodata <=> Audiodata
std::optional<Spectrodata> audio_to_spectro(const Audiodata& in, samples_t window, samples_t step);
std::optional<Audiodata> spectro_to_audio(const Spectrodata& in);
Imagedata_MONO spectro_to_image(const Spectrodata& in);
Spectrodata image_to_spectro(const Imagedata_MONO& in);

// Read operations
int audio_probe_channels(std::string fname);
std::optional<Audiodata> audio_read_mono(std::string fname);
std::optional<Audiodata_STEREO> audio_read_stereo(std::string fname);

std::optional<Spectrodata> spectro_read(std::string fname);

int image_probe_channels(std::string fname);
std::optional<Imagedata_MONO> image_read_mono(std::string fname);
std::optional<Imagedata_MONOA> image_read_monoa(std::string fname);
std::optional<Imagedata_RGB> image_read_rgb(std::string fname);
std::optional<Imagedata_RGBA> image_read_rgba(std::string fname);

// Write operations
int audio_write_mono(std::string fname, const Audiodata& audio);
int audio_write_stereo(std::string fname, const Audiodata_STEREO& audio);
int audio_write_stereo(std::string fname, const Audiodata& left, const Audiodata& right);

int spectro_write(std::string fname, const Spectrodata& spectro);

int image_write_mono(std::string fname, const Imagedata_MONO& image);
int image_write_monoa(std::string fname, const Imagedata_MONOA& image);
int image_write_rgb(std::string fname, const Imagedata_RGB& image);
int image_write_rgba(std::string fname, const Imagedata_RGBA& image);
}