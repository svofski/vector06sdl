#include "8253.h"
#include "ay.h"
#include "biquad.h"
#include "SDL.h"
#include "wav.h"
#include "options.h"
#include "sound.h"

#define BIQUAD_FLOAT 1

void Soundnik::init(WavRecorder * _rec) 
{
    this->rec = _rec;

    butt1 = new Bypass();
    butt2 = new Bypass();
    if (Options.nosound) {
        return;
    }

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 2;

    if (!Options.vsync) {
        this->sound_frame_size = want.freq / 50;
    }

    want.samples = this->sound_frame_size;
    want.callback = Soundnik::callback;  // you wrote this function elsewhere.
    want.userdata = (void *)this;

    SDL_Init(SDL_INIT_AUDIO);

    if(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL audio error: SDL_INIT_AUDIO not initialized\n");
        return;
    }

    if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 
            SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
        fprintf(stderr, "SDL audio error: %s", SDL_GetError());
        Options.nosound = true;
        return;
    };

    if (have.samples == this->sound_frame_size / 2) {
        // strange thing but we get a half buffer, try to get 2x
        SDL_CloseAudioDevice(this->audiodev);

        Options.log.audio &&
        fprintf(stderr, "SDL audio: retrying to open device with 2x buffer size\n");
        want.samples = this->sound_frame_size * 2;

        if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 
                        SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
            fprintf(stderr, "SDL audio error: %s", SDL_GetError());
            Options.nosound = true;
            return;
        };

        if (have.samples < this->sound_frame_size) {
            fprintf(stderr, "SDL audio cannot get the right buffer size, giving up\n");
            Options.nosound = true;
        }
    }

    this->sampleRate = have.freq;

    // one second = 50 frames
    // raster time in 12 MHz pixelclocks = 768 columns by 312 lines
    // timer clocks = pixel clock / 8
    int timer_cycles_per_second = 50*768*312/8; // 1497600
    // for 48000: 3120
    // for 44100: 3396, which gives 44098.9... 
    this->sound_accu_top = (int)(0.5 + 100.0 * timer_cycles_per_second / this->sampleRate); 
    this->sound_accu_int = 0;

    Options.log.audio && 
    printf("SDL audio dev: %d, sample rate=%d "
            "have.samples=%d have.channels=%d have.format=%d have.size=%d\n", 
            this->audiodev, this->sampleRate, 
            have.samples, have.channels, have.format, have.size);

    // filters
    if (!Options.nofilter) {
        butt1 = new Biquad();
        butt2 = new Biquad();
        butt1->ba(0.00021253813256227462, 0.00042507626512454925, 0.00021253813256227462, 
                -1.9769602848645957, 0.9778104373948449);
        butt2->ba(0.0002092548424353858, 0.0004185096848707716, 0.0002092548424353858,
                -1.9464201925701206, 0.9472572119398622);


        butt1->calcLowpass(1500000, 10000.0f, 0.7f); // SRC filter
        butt2->calcLowpass(this->sampleRate, 6000.0f, 0.7f); // whistle filter


        // jpcima's fast-filter
        // N=4
        // [b,a] = cheby2(3,60,50000/1.5e6);
        // only stable with doubles
        float cheby2b[] = {
            0.0001552034358995624827113474220041666740144137293100357055664062,
            -0.0001529382489585779335544007961900092595897149294614791870117188,
            -0.0001529382489585779335544007961900092595897149294614791870117188,
            0.0001552034358995624827113474220041666740144137293100357055664062,
        };
        float cheby2a[] = {
            1.0000000000000000000000000000000000000000000000000000000000000000,
            -2.9668277758033450020036525529576465487480163574218750000000000000,
            2.9342034839429493864315645623719319701194763183593750000000000000,
            -0.9673711777657223453985579908476211130619049072265625000000000000,
        };
        resample_iir.reset();
        resample_iir.coefs(cheby2b, cheby2a);
    }
    butt1->calcInteger();
    butt2->calcInteger();
}




void Soundnik::soundStep(int step, int tapeout, int covox, int tapein)
{
    float ay = this->aywrapper.step2(step);

    /* timerwrapper does the stepping of 8253, it must always be called */
#if BIQUAD_FLOAT
    float soundf = this->timerwrapper.step(step/2) + tapeout + tapein;
    if (Options.nosound) return; /* but then we can return if nosound */
    //soundf = this->butt2->ffilter(this->butt1->ffilter(soundf * 0.2f));
    //soundf = this->butt1->ffilter_2stage(soundf * 0.2f);
    soundf = this->resample_iir.tick(soundf * 0.2f);
    //printf("%f\n", soundf);
#else
    int soundi = (this->timerwrapper.step(step / 2) + tapeout + tapein) << 21;
    if (Options.nosound) return;
    //soundi = this->butt2->ifilter(this->butt1->ifilter(soundi));

#endif

    this->sound_accu_int += 100;
    if (this->sound_accu_int >= this->sound_accu_top) {
        this->sound_accu_int -= this->sound_accu_top;

#if BIQUAD_FLOAT
        //soundf = this->butt2->ffilter(soundf);
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


