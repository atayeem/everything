#include <stdio.h>
#include <sndfile.h>
#include <fftw3.h>

SF_INFO file_mode = {
    .samplerate = 44100,
    .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    .channels = 1,
};

int main() {
    SNDFILE* the_file = sf_open("out.wav", SFM_WRITE, &file_mode);
    
    if (the_file == NULL) {
        puts(sf_strerror(NULL));
        return 1;
    }

    sf_close(the_file);
    return 0;
}