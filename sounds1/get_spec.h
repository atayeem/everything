#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <fftw3.h>

int get_spec_init();
int get_spec(const double *in, double *out);