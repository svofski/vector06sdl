#include "8253.h"
#include "ay.h"
#include "biquad.h"
#include "SDL.h"
#include "wav.h"
#include "options.h"
#include "sound.h"

#include "resampler.h"

#define BIQUAD_FLOAT 1

static int print_driver_info() 
{
    /* Print available audio drivers */
    int n = SDL_GetNumAudioDrivers();
    if (n == 0) {
        SDL_Log("No built-in audio drivers\n\n");
    } else {
        int i;
        SDL_Log("Built-in audio drivers:\n");
        for (i = 0; i < n; ++i) {
            SDL_Log("  %d: %s\n", i, SDL_GetAudioDriver(i));
        }
        SDL_Log("Select a driver with the SDL_AUDIODRIVER environment variable.\n");
    }

    SDL_Log("Using audio driver: %s\n\n", SDL_GetCurrentAudioDriver());
    return 1;
}

void Soundnik::init(WavRecorder * _rec) 
{
    this->rec = _rec;

    if (Options.nosound) {
        return;
    }

    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want)); 
    want.freq = 48000;
    want.format = AUDIO_F32;
    want.channels = 2;

    //if (!Options.vsync) {
        this->sound_frame_size = want.freq / 50;
    //}

    want.samples = this->sound_frame_size;
    want.callback = Soundnik::callback; 
    want.userdata = (void *)this;

    SDL_Init(SDL_INIT_AUDIO);

    Options.log.audio && ::print_driver_info();

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
    if (Options.nofilter) {
        resampler.set_passthrough(true);
    }
}

void Soundnik::soundStep(int step, int tapeout, int covox, int tapein)
{
    float ay = this->aywrapper.step2(step);

    /* timerwrapper does the stepping of 8253, it must always be called */
#if BIQUAD_FLOAT
    float soundf = this->timerwrapper.step(step/2) + tapeout + tapein;
    if (Options.nosound) return; /* but then we can return if nosound */
    soundf = this->resampler.sample(soundf * 0.2f);
#else
    int soundi = (this->timerwrapper.step(step / 2) + tapeout + tapein) << 21;
    if (Options.nosound) return;

#endif

    //static int between = 0;

    this->sound_accu_int += 100;
    //between++;
    //if (this->sound_accu_int >= this->sound_accu_top) {
    //    this->sound_accu_int -= this->sound_accu_top;
        //printf("[%d] ", between); between = 0;
    if (resampler.egg) {
        resampler.egg = false;
#if BIQUAD_FLOAT
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

void Soundnik::pause(int pause)
{
    if (!Options.nosound) {
        SDL_PauseAudioDevice(this->audiodev, pause);
    }
    this->wrptr = 0;
    this->rdbuf = 0;
    this->wrbuf = 0;
    this->sound_accu_int = 0;
}

static int manquator = 0;

void Soundnik::callback(void * userdata, uint8_t * stream, int len)
{
    Soundnik * that = (Soundnik *)userdata;
    float * fstream = (float *)stream;

    if (that->rdbuf == that->wrbuf) {
        memcpy(stream, that->buffer[that->rdbuf], that->wrptr * sizeof(float));
        for (int i = that->wrptr, end = that->sound_frame_size * 2; i < end; ++i) {
            fstream[i] = that->last_value;
        }
        if (++manquator == 10) {
            printf("SPECIAL PLACe\n");
        }
        Options.log.audio &&
            fprintf(stderr, "starve rdbuf=%d wrbuf=%d en manque=%d\n", 
                    that->rdbuf, that->wrbuf, that->sound_frame_size - that->wrptr/2);
        that->wrptr = 0;
    } else
    {
        memcpy(stream, that->buffer[that->rdbuf], len);
        if (++that->rdbuf == Soundnik::NBUFFERS) {
            that->rdbuf = 0;
        }
    }

    that->rec &&
        that->rec->record_buffer(fstream, that->sound_frame_size * 2);

    /* sound callback is also our frame interrupt source */
    if (!Options.vsync) {
        extern uint32_t timer_callback(uint32_t interval, void * param);
        timer_callback(0, 0);
        putchar('s'); fflush(stdout);
    }
}

void Soundnik::sample(float samp) 
{
    if (!Options.nosound) {
        SDL_LockAudioDevice(this->audiodev);
        this->last_value = samp;
        this->buffer[this->wrbuf][this->wrptr++] = samp;
        this->buffer[this->wrbuf][this->wrptr++] = samp;
        if (this->wrptr >= this->sound_frame_size * 2) {
            this->wrptr = 0;
            if (++this->wrbuf == Soundnik::NBUFFERS) {
                this->wrbuf = 0;
            }
        }
        SDL_UnlockAudioDevice(this->audiodev);
    }
}

