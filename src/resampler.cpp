#include "resampler.h"
#include "biquad.h"
#include "coredsp/filter.h"

#define SAVERAW 0
#define SAVERAW_DOWNSAMPLED 0

#if SAVERAW
#include "stdio.h"
float buf[8192];
int   bptr = 0;
FILE *raw;

#endif

#define DECIMATE 5

using namespace std;

typedef coredsp::FIR<1281, coreutil::simd_t<float>> fir_t;

void init_interp(fir_t & filter)
{
#include "../filters/interp.h"
    filter.coefs(coefs);
}

Resampler::Resampler() 
{
    create_filter();
    thru = false;
#if SAVERAW
    raw = fopen("float32.raw", "wb");
#endif
}


void Resampler::create_filter()
{
    for (int i = 0; i < nlevels; ++i) {
        fir_t * fir = new fir_t();
        init_interp(*fir);
        this->f[i] = fir;

        ctr[i] = 0;
    }
    dcm_ctr = 0;
}

float Resampler::sample(float s)
{
#if SAVERAW
    buf[bptr++] = s;
    if (bptr >= 8192) {
        bptr = 0;
        fwrite(buf, 1, sizeof(buf), raw);
    }
#endif
    if (!this->thru) {
        for (int i = 0; i < 5; ++i) {
            ((fir_t *)f[0])->in(i==0?s:0);
            if (++dcm_ctr == 156) {
                dcm_ctr = 0;
                this->in[nlevels] = 5 * ((fir_t *)f[0])->out();
                this->egg = true;
            }
        }
    }
    else {
        this->in[nlevels] = s;
    }

    return this->in[nlevels];
}

Resampler::~Resampler()
{
    for (int i = 0; i < nlevels; ++i) {
        delete (fir_t *)this->f[i];
    }

#if SAVERAW
    fclose(raw);
#endif
}

void Resampler::set_passthrough(bool thru_) 
{
    this->thru = thru_;
}
