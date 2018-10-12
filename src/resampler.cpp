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

using namespace std;

static constexpr int ALLTAPS = 911;
static constexpr int NTAPS = (ALLTAPS+Resampler::UP)/Resampler::UP;
typedef coredsp::FIR<NTAPS, coreutil::simd_t<float>> fir_t;

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
#include "../filters/interp.h"

    double padded[ALLTAPS + UP];
    double n[NTAPS];

    for (int i = 0; i < ALLTAPS; ++i) padded[i] = coefs[i];
    for (int i = ALLTAPS; i < ALLTAPS + UP; ++i) padded[i] = 0;

    for (int phase = 0; phase < UP; ++phase) {
        for (int i = phase, k = 0; i < ALLTAPS; i += UP, k += 1) {
            n[k] = padded[i];
        }
        fir_t * fir = new fir_t();
        fir->coefs(n);
        this->filterbank[phase] = fir;
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
        /* filterbank at input samplerate */
        for (int i = 0; i < UP; ++i) {
            ((fir_t *)filterbank[i])->in(s);
        }

        /* phase commutator works at samplerate * UP */
        for (int i = 0; i < UP; ++i) {
            if (++dcm_ctr == DOWN) {
                dcm_ctr = 0;
                this->out = ((fir_t *)filterbank[i])->out();
                this->egg = true;
            }
        }
        //dcm_ctr += UP;
        //if (dcm_ctr >= DOWN) {
        //    dcm_ctr -= DOWN;
        //    -- what is really the phase here -- 
        //    this->out = ((fir_t *)filterbank[??])->out();
        //    this->egg = true;
        //}
    }
    else {
        this->out = s;
    }

    return this->out;
}

Resampler::~Resampler()
{
    for (int i = 0; i < UP; ++i) {
        delete (fir_t *)this->filterbank[i];
    }

#if SAVERAW
    fclose(raw);
#endif
}

void Resampler::set_passthrough(bool thru_) 
{
    this->thru = thru_;
}
