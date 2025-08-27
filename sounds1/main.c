#include <stdio.h>
#include <sndfile.h>
#include <fftw3.h>
#include <string.h>

const double SAMPLE_RATE = 44100; // samples per second

SF_INFO file_mode = {
    .samplerate = SAMPLE_RATE,
    .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    .channels = 1,
};

const int N = 1 << 20; // number of samples
const int N_OUT = 2 * N + 1;

#define RE 0
#define IM 1

void add_frequency(fftw_complex *array, size_t array_size, double freq) {
    size_t k = (int)(freq * array_size / SAMPLE_RATE);

    array[k][IM] = -10000;
    array[N-k][IM] = 10000;
}

int main() {
    printf("Using %s\n", sf_version_string());
    printf("Using %s\n", fftw_version);

    SNDFILE* the_file = sf_open("out.wav", SFM_WRITE, &file_mode);

    if (the_file == NULL) {
        puts(sf_strerror(NULL));
        return 1;
    }

    fftw_complex *in;
    double *out;
    fftw_plan p;
    
    in = fftw_malloc(sizeof(fftw_complex) * N);
    out = fftw_malloc(sizeof(double) * (N_OUT));

    if (!in || !out) {
        puts("Failed to allocate memory!");
        return 1;
    }

    // initialize the data
    memset(in, 0, sizeof(fftw_complex) * N);
    memset(out, 0, sizeof(double) * N_OUT);

    add_frequency(in, N, 440.0);
    add_frequency(in, N, 550.0);
    add_frequency(in, N, 10000.0);

    // Set a single frequency bin for the sine wave
    p = fftw_plan_dft_c2r_1d(N, in, out, FFTW_ESTIMATE);

    fftw_execute(p);

    for (int i = 0; i < N; i++)
        out[i] /= N;

    fftw_destroy_plan(p);
    fftw_free(in);

    sf_count_t written = sf_write_double(the_file, out, N_OUT);

    if (written != 2*N + 1) {
        puts("Error writing samples.\n");
    }

    fftw_free(out);

    sf_close(the_file);
    return 0;
}