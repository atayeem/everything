#include "libasi.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [audio file] [png file]\n";
        return -1;
    }
    char *fname = argv[1];
    char *iname = argv[2];

    int channels = asi::audio_probe_channels(fname);

    asi::Audiodata audio;

    if (channels == 1) {
        audio = asi::audio_read_mono(fname).value();
    
    } else if (channels == 2) {
        audio = asi::audio_read_stereo(fname).value().left;
    
    } else {
        std::cerr << "Audio file doesn't exist or has more than 2 channels.\n";
        return -1;
    }

    asi::Spectrodata spectro = asi::audio_to_spectro(audio, 2048, 1024).value();
    asi::Imagedata_MONO image = asi::spectro_to_image(spectro);
    
    if (!asi::image_write_mono(iname, image)) {
        std::cerr << "Failed to write image.\n";
        return 1;
    }

    return 0;
}