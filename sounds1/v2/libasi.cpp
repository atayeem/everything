#include "libasi.hpp"
#include <iostream>

#include <sndfile.h>
#include <fftw3.h>

using namespace asi;

Spectrodata audio_to_spectro(Audiodata audio, samples_t window, samples_t step) {
    Spectrodata spectro;
    spectro.step = step;
    spectro.window = window;

    std::vector<double> audio_copy(audio.data);


    fftw_plan p = fftw_plan_dft_r2c_1d(audio.data.size(), audio_copy.data(), )
    return spectro;
}

Audiodata spectro_to_audio(Spectrodata in, samples_t sample_rate);
Imagedata_MONO spectro_to_image(Spectrodata in);
Spectrodata image_to_spectro(Imagedata_MONO in);

int audio_probe_channels(std::string fname) {
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

std::optional<Audiodata> audio_read_mono(std::string fname) {
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
    audio.data.reserve(sfinfo.frames);
    (void) sf_readf_double(sndfile, audio.data.data(), sfinfo.frames);
    audio.sample_rate = sfinfo.samplerate;

    return audio;
}

std::optional<Audiodata_STEREO> audio_read_stereo(std::string fname) {
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

    left.data.reserve(sfinfo.frames);
    right.data.reserve(sfinfo.frames);

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