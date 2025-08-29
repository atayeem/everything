#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include <fftw3.h>
#include <math.h>
#include <string.h>

#define RE 0
#define IM 1

const int SAMPLE_RATE_HZ = 44100;

const int FILE_DURATION_SMP = 12 * SAMPLE_RATE_HZ; // seconds

const int WINDOW_SIZE_SMP = 2048;
const int HOP_SIZE_SMP = WINDOW_SIZE_SMP / 2;

SF_INFO file_mode = {
    .samplerate = SAMPLE_RATE_HZ,
    .format = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    .channels = 1,
};

void generate_han_window(double *w) {
    for (int n = 0; n < WINDOW_SIZE_SMP; n++) {
        w[n] = 0.5 * (1 - cos(2.0 * M_PI * n / (WINDOW_SIZE_SMP - 1)));
    }
}

void add_frequency(fftw_complex *array, double freq, double power) {
    int index = (int)(freq * WINDOW_SIZE_SMP / SAMPLE_RATE_HZ);

    array[index][IM] = -power;
    array[WINDOW_SIZE_SMP - index][IM] = power;
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
    fftw_plan p;
    double *out;
    double *w;
    double *final;
    
    in = fftw_malloc(sizeof(fftw_complex) * WINDOW_SIZE_SMP);
    out = fftw_malloc(sizeof(double) * WINDOW_SIZE_SMP);
    p = fftw_plan_dft_c2r_1d(WINDOW_SIZE_SMP, in, out, FFTW_ESTIMATE);
    w = calloc(WINDOW_SIZE_SMP, sizeof(double));
    final = calloc(FILE_DURATION_SMP, sizeof(double));

    if (!in || !out) {
        puts("fftw_malloc failed to allocate memory!");
        return 1;
    }

    if (!p) {
        puts("fftw_plan_dft_c2r_1d failed to generate a plan!");
        return 1;
    }

    if (!final || !w) {
        puts("calloc failed to allocate memory!");
        return 1;
    }

    generate_han_window(w);

    /*
    Steps:
        1. Write zeroes to `in` and `out`.
        2. Every time, create a new frequency space in `in`, filling in the frequency bins.
        3. Execute the IFFT, writing to `out`.
        4. Divide `out` by the window size to normalize it.
        5. Multiply `out` by the hanning window.
        6. Accumulate it in the output buffer in the right place
    */
    for (int pos = 0; pos < FILE_DURATION_SMP; pos += WINDOW_SIZE_SMP) {
        memset(in, 0, sizeof(fftw_complex) * WINDOW_SIZE_SMP);
        memset(out, 0, sizeof(double) * WINDOW_SIZE_SMP);

        add_frequency(in, 880 * pow(2, floor(12.0 * pos / FILE_DURATION_SMP) / 12), 40);

        fftw_execute(p);

        for (int j = 0; (j < WINDOW_SIZE_SMP) && (pos + j < FILE_DURATION_SMP); j++) {
            // printf("Writing %d\n", pos + j);
            final[pos + j] += out[j] / WINDOW_SIZE_SMP * 1;
        }
    }

    sf_count_t written = sf_write_double(the_file, final, FILE_DURATION_SMP);

    if (written != FILE_DURATION_SMP)
        puts("Error writing samples.\n");

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
    free(w);
    free(final);

    sf_close(the_file);

    return 0;
}