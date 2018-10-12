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

static constexpr int ALLTAPS = 1285;
static constexpr int NTAPS = ALLTAPS/Resampler::UP;
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

    double n[NTAPS];
    double maxsum = -100500;
    double sum[UP];

    for (int phase = 0; phase < UP; ++phase) {
        sum[phase] = 0;
        for (int i = phase, k = 0; i < ALLTAPS; i += UP, k += 1) {
            sum[phase] += coefs[i];
        }
        printf("phase %d sum=%f\n", phase, sum[phase]);
        if (sum[phase] > maxsum) maxsum = sum[phase];
    }


    for (int phase = 0; phase < UP; ++phase) {
        double ratio = maxsum/sum[phase];
        sum[phase] = 0;
        for (int i = phase, k = 0; i < ALLTAPS; i += UP, k += 1) {
            n[k] = coefs[i] * ratio;
            sum[phase] += n[k];
        }
        fir_t * fir = new fir_t();
        fir->coefs(n);
        this->filterbank[phase] = fir;
        printf("phase %d sum=%f\n", phase, sum[phase]);
    }

    dcm_ctr = 0;
    phase = 0;
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
