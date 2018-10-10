#include "resampler.h"
#include "biquad.h"
#include "coredsp/filter.h"

#define SAVERAW 0

#if SAVERAW
#include "stdio.h"
float buf[8192];
int   bptr = 0;
FILE *raw;

#endif

#define DECIMATE 2

using namespace std;

typedef coredsp::FIR<33, coreutil::simd_t<float>> fir_t;

void init_halfband(fir_t & filter)
{
#include "../filters/halfband.h"
    filter.coefs(coefs);
}

void init_endstage(fir_t & filter)
{
#include "../filters/endstage.h"
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
        if (i < nlevels - 1) {
            init_halfband(*fir);
        } 
        else {
            init_endstage(*fir);
        }
        this->f[i] = fir;

        ctr[i] = 0;
    }
}

/* 6 cascades of half-band filters (the last one has narrower passband) 
 * Each level always takes in an input, but computes output only when
 * the result is needed. This means 1/2 multiplications per cascade.
 */
float Resampler::sample(float s)
{
#if 0 & SAVERAW
    buf[bptr++] = s;
    if (bptr >= 8192) {
        bptr = 0;
        fwrite(buf, 1, sizeof(buf), raw);
    }
#endif
    in[0] = s;
    if (!this->thru) {
        for (int level = 0; level < nlevels; ++level) {
            ((fir_t *)f[level])->in(in[level]);         // take a sample
            if (++ctr[level] == DECIMATE) {
                ctr[level] = 0;
                in[level+1] = ((fir_t *)f[level])->out(); // calculate stage output
#if SAVERAW
                if (level == nlevels - 1) {
                    buf[bptr++] = in[nlevels];
                    if (bptr >= 8192) {
                        bptr = 0;
                        fwrite(buf, 1, sizeof(buf), raw);
                    }
                }
#endif
                 continue;
            }
            break;
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
