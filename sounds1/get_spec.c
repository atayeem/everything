#include "get_spec.h"
#include <math.h>

#define RE 0
#define IM 1

#define SAMPLE_RATE_HZ 44100
#define SZ 2048

static double w[SZ];
static double *transform_in;
static fftw_complex *transform_out;
static fftw_plan p;

int get_spec_init() 
{
    // Generate window function
    for (int n = 0; n < SZ; n++)
        w[n] = 0.5 * (1 - cos(2.0 * M_PI * n / (SZ - 1)));
    
    transform_in = fftw_malloc(sizeof(double) * SZ);
    transform_out = fftw_malloc(sizeof(fftw_complex) * SZ);

    if (!transform_in || !transform_out)
        return 0;

    p = fftw_plan_dft_r2c_1d(SZ, transform_in, transform_out, FFTW_ESTIMATE);

    if (!p)
        return 0;

    return 1;
}

int get_spec(const double *in, double *out)
{
    double *final;

    for (int i = 0; i < SZ; i++)
        transform_in[i] = in[i] * w[i];

    fftw_execute(p);

    memcpy(out, transform_out, sizeof(double) * SZ);
}