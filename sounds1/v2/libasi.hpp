#pragma once

#include <complex>
#include <cstdint>
#include <optional>
#include <vector>
#include <cstdlib>

typedef size_t samples_t;

namespace asi {
    
    struct Audiodata {
        std::vector<double> data;
        samples_t sample_rate;
    };

    struct Audiodata_STEREO {
        Audiodata left; 
        Audiodata right;
    };

    struct Spectrodata {
        std::vector<std::complex<double>> data;
        samples_t window;
        samples_t step;
    };

    struct Imagedata_MONO {
        std::vector<uint8_t> data;
        size_t width;
        size_t height;
    };

    struct Imagedata_RGBA {
        std::vector<uint32_t> data;
        size_t width;
        size_t height;
    };

    // Conversion of the 3 basic types
    // Imagedata_MONO <=> Spectrodata <=> Audiodata
    Spectrodata audio_to_spectro(Audiodata in, samples_t window, samples_t step);
    Audiodata spectro_to_audio(Spectrodata in, samples_t sample_rate);
    Imagedata_MONO spectro_to_image(Spectrodata in);
    Spectrodata image_to_spectro(Imagedata_MONO in);

    // Read operations
    int audio_probe_channels(std::string fname);
    std::optional<Audiodata> audio_read_mono(std::string fname);
    std::optional<Audiodata_STEREO> audio_read_stereo(std::string fname);

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
}