#pragma once

#include <atomic>

#include "8253.h"
#include "ay.h"
#include "biquad.h"
#include "SDL.h"

class Soundnik
{
private:
    TimerWrapper & timerwrapper;
    AYWrapper & aywrapper;
    SDL_AudioDeviceID audiodev;

    static const int buffer_size = 2048 * 2; // 96000/50=1920, enough
    int sound_frame_size = 2048;

    static const int NBUFFERS = 8;
    float buffer[NBUFFERS][buffer_size];
    static const int mask = buffer_size - 1;
    std::atomic_int wrptr;
    int wrbuf;
    int rdbuf;
    float last_value;

    int sampleRate;

    int sound_accu_int, sound_accu_top;

    Biquad butt1;
    Biquad butt2;

public:
    Soundnik(TimerWrapper & tw, AYWrapper & aw) : timerwrapper(tw), 
        aywrapper(aw)
    {}

    void init() {
        SDL_AudioSpec want, have;

        if (Options.nosound) {
            return;
        }

        SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
        want.freq = 48000;
        want.format = AUDIO_F32;
        want.channels = 2;

        //this->sound_frame_size = want.freq / 50;

        want.samples = this->sound_frame_size;
        want.callback = Soundnik::callback;  // you wrote this function elsewhere.
        want.userdata = (void *)this;

        SDL_Init(SDL_INIT_AUDIO);

        if(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
            printf("SDL audio error: SDL_INIT_AUDIO not initialized\n");
            return;
        }

        if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 
                SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
            printf("SDL audio error: %s", SDL_GetError());
            Options.nosound = true;
            return;
        };

        if (have.samples == this->sound_frame_size / 2) {
            // strange thing but we get a half buffer, try to get 2x
            SDL_CloseAudioDevice(this->audiodev);

            printf("SDL audio: retrying to open device with 2x buffer size\n");
            want.samples = this->sound_frame_size * 2;

            if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 
                            SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
                printf("SDL audio error: %s", SDL_GetError());
                Options.nosound = true;
                return;
            };

            if (have.samples < this->sound_frame_size) {
                printf("SDL audio cannot get the right buffer size, giving up\n");
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

        printf("SDL audio dev: %d, sample rate=%d "
                "have.samples=%d have.channels=%d have.format=%d have.size=%d\n", 
                this->audiodev, this->sampleRate, 
                have.samples, have.channels, have.format, have.size);

        // filters
        //butt1.calcLowpass(this->sampleRate, 1500, 1.0);
        //butt2.calcLowpass(this->sampleRate, 2000, 1.5);
        butt1.ba(0.00021253813256227462, 0.00042507626512454925, 0.00021253813256227462, 
                -1.9769602848645957, 0.9778104373948449);
        butt2.ba(0.0002092548424353858, 0.0004185096848707716, 0.0002092548424353858,
                -1.9464201925701206, 0.9472572119398622);

        butt1.calcInteger();
        butt2.calcInteger();

    }

    void pause(int pause)
    {
        SDL_PauseAudioDevice(this->audiodev, pause);
        this->wrptr = 0;
        this->rdbuf = 0;
        this->wrbuf = 0;
        this->sound_accu_int = 0;
    }

    static void callback(void * userdata, uint8_t * stream, int len)
    {
        Soundnik * that = (Soundnik *)userdata;
        float * fstream = (float *)stream;

        if (that->rdbuf == that->wrbuf) {
            memcpy(stream, that->buffer[that->rdbuf], that->wrptr * sizeof(float));
            for (int i = that->wrptr, end = that->sound_frame_size * 2; i < end; ++i) {
                fstream[i] = that->last_value;
            }
            printf("starve rdbuf=%d wrbuf=%d en manque=%d\n", that->rdbuf, that->wrbuf, 
                    that->sound_frame_size - that->wrptr/2);
            that->wrptr = 0;
        } else {
            memcpy(stream, that->buffer[that->rdbuf], len);
            if (++that->rdbuf == Soundnik::NBUFFERS) {
                that->rdbuf = 0;
            }
        }

        if (!Options.vsync) {
            /* sound callback is also our frame interrupt source */
            extern uint32_t timer_callback(uint32_t interval, void * param);
            timer_callback(0, 0);
        }
    }

    void sample(float samp) 
    {
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

#define BIQUAD_FLOAT 1

    void soundStep(int step, int tapeout, int covox, int tapein) 
    {
        float ay = this->aywrapper.step2(step);

#if BIQUAD_FLOAT
        float soundf = (this->timerwrapper.step(step/2) + tapeout + tapein) * 0.2 + ay*0.7 - 0.5;
        soundf = this->butt2.ffilter(this->butt1.ffilter(soundf));
#else
        int soundi = (this->timerwrapper.step(step / 2) + tapeout + tapein) << (32-8);
        soundi += (int)(ay * (162777216.0/4));
        soundi = this->butt2.ifilter(this->butt1.ifilter(soundi));

#endif

        this->sound_accu_int += 100;
        if (this->sound_accu_int >= this->sound_accu_top) {
            this->sound_accu_int -= this->sound_accu_top;

#if BIQUAD_FLOAT
            float sound = soundf + covox/256.0;
#else
            float sound = soundi / 16277216.0 + covox/256.0;
#endif
            sound = (sound - 1.5f) * 0.3f;
            if (sound > 1.0f) { 
                sound = 1.0f; 
            } else if (sound < -1.0f) { 
                sound = -1.0f; 
            }
            this->sample(sound);
        }
    }

};

