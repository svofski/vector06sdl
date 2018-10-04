#include "8253.h"
#include "ay.h"
#include "biquad.h"
#include "SDL.h"
#include "wav.h"
#include "options.h"
#include "sound.h"

#define BIQUAD_FLOAT 1
void Soundnik::soundStep(int step, int tapeout, int covox, int tapein)
{
    float ay = this->aywrapper.step2(step);

    /* timerwrapper does the stepping of 8253, it must always be called */
#if BIQUAD_FLOAT
    float soundf = this->timerwrapper.step(step/2) + tapeout + tapein;
    if (Options.nosound) return; /* but then we can return if nosound */
    //soundf = this->butt2->ffilter(this->butt1->ffilter(soundf * 0.2f));
    soundf = this->butt1->ffilter_2stage(soundf * 0.2f);
#else
    int soundi = (this->timerwrapper.step(step / 2) + tapeout + tapein) << 21;
    if (Options.nosound) return;
    //soundi = this->butt2->ifilter(this->butt1->ifilter(soundi));

#endif

    this->sound_accu_int += 100;
    if (this->sound_accu_int >= this->sound_accu_top) {
        this->sound_accu_int -= this->sound_accu_top;

#if BIQUAD_FLOAT
        //soundf = Biquad::ffilter_buf(this->filterbuf, this->filteridx, 
        //        static_cast<Biquad&>(*this->butt1), 
        //        static_cast<Biquad&>(*this->butt2));
        //soundf = this->butt1->ffilter_2stage(this->filterbuf, this->filteridx);
        this->filteridx = 0;

        float sound = soundf + ay * 0.2;// + covox/256.0;
#else
        float sound = soundi / 16277216.0 + ay * 0.2;// + covox/256.0;
#endif
        if (sound > 1.0f) { 
            sound = 1.0f; 
        } else if (sound < -1.0f) { 
            sound = -1.0f; 
        }
        this->sample(sound);
    }
}


