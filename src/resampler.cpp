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

/* Nice filter parameters calculated as:
 * L=5; M=156
 * N=182*L
 * fw = signal.firwin(N+1, .6/M, window=('kaiser', 7.8562))
 *
 * But this gives plenty of zeroes at both ends, so:
 *
 * trim=fw[214:len(fw)-214]
 *
 * And from 911 taps we arrive at 483 taps. Divided by 5 phases
 * this makes only 97 taps per phase.
 *
 * NB: firwin filter is symmetrical so this could also be folded.
 */
static constexpr int ALLTAPS = 483;
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

    float padded[ALLTAPS + UP];
    float n[NTAPS];

    for (int i = 0; i < ALLTAPS; ++i) padded[i] = coefs[i];
    for (int i = ALLTAPS; i < ALLTAPS + UP; ++i) padded[i] = 0;

    for (int phase = 0; phase < UP; ++phase) {
        for (int i = phase, k = 0; k < NTAPS; i += UP, k += 1) {
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
        /* Rational resampler as described in 
         * "Introduction to Digital Resampling" by Dr. Mike Porteous
         * Upsample UP times by zero insetions, interpolate using FIR filter,
         * then pick every DOWN sample.
         * FIR filter is decomposed in a bank of UP phase filters. Each phase
         * is fed at the input samplerate. The output of interpolator is
         * selected by a commutator running at the rate of input * UP.
         *
         * Only the filter required for the current output is computed.
         * So this implementation is pretty efficient. 
         */

        /* Feed filters at input samplerate, only push the samples */
        for (int i = 0; i < UP; ++i) {
            ((fir_t *)filterbank[i])->in(s);
        }

        /* Pick the output phase: the  commutator runs  at samplerate * UP.
         * This part is merged with the decimator. When the output sample
         * is needed, it is computed from the corresponding phase filter.
         */
#if 0
        for (int i = 0; i < UP; ++i) {
            if (++dcm_ctr == DOWN) {
                dcm_ctr = 0;
                this->out = UP * ((fir_t *)filterbank[i])->out();
                this->egg = true;
            }
        }
#else
        dcm_ctr += UP;
        if (dcm_ctr >= DOWN) {
            dcm_ctr -= DOWN;
            int phase = UP - dcm_ctr - 1;
            this->out = UP * ((fir_t *)filterbank[phase])->out();
            this->egg = true;
        }
#endif
    }
    else {
        dcm_ctr += UP;
        if (dcm_ctr >= DOWN) {
            dcm_ctr -= DOWN;
            this->egg = true;
        }
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
