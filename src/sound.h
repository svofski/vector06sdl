#pragma once

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

    static const int renderingBufferSize = 8192;
    int sdlBufferSize = 960;

    float renderingBuffer[renderingBufferSize];
    static const int mask = renderingBufferSize - 1;
    int sndCount;
    int sndReadCount;
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

        this->sdlBufferSize = want.freq / 50;

        want.samples = sdlBufferSize;
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

        if (have.samples == sdlBufferSize / 2) {
            // strange thing but we get a half buffer, try to get 2x
            SDL_CloseAudioDevice(this->audiodev);

            printf("SDL audio: retrying to open device with 2x buffer size\n");
            want.samples = sdlBufferSize * 2;

            if ((this->audiodev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 
                            SDL_AUDIO_ALLOW_FORMAT_CHANGE)) == 0) {
                printf("SDL audio error: %s", SDL_GetError());
                Options.nosound = true;
                return;
            };

            if (have.samples < sdlBufferSize) {
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
        this->sndReadCount = this->sndCount = 0;
        this->sound_accu_int = 0;
    }

    static void callback(void * userdata, uint8_t * stream, int len)
    {
        static float last_value;

        Soundnik * that = (Soundnik *)userdata;
        int diff = (that->sndCount - that->sndReadCount) & that->mask;
        float * fs = (float *)stream;
        int src = that->sndReadCount;
        int count = len / sizeof(float) / 2;
        if (diff < count) {
            --that->sound_accu_top;
            printf("audio starved: have=%d need=%d top=%d\n", diff, count,
                    that->sound_accu_top);
            // We're running short of samples.
            // Skip this frame completely, this will increase the latency,
            // but the hiccup rate should be lower on average
            memset(stream, 0, len);
            for (int i = 0; i < count*2; i += 2) {
                fs[i] = last_value;
                fs[i+1] = last_value;
            }
        } else {
            int end = count;
            if (diff < end) end = diff;
            int i, dst;
            for (i = 0, dst = 0; i < end; ++i) {
                last_value = that->renderingBuffer[src];
                fs[dst++] = last_value; // Left
                fs[dst++] = last_value; // Right
                src = (src + 1) & that->mask;
            }
            for (; i < count; ++i) {
                fs[dst++] = last_value;
                fs[dst++] = last_value;
            }
            that->sndReadCount = src;
        }

        /* sound callback is also our frame interrupt source */
        extern uint32_t timer_callback(uint32_t interval, void * param);
        timer_callback(0, 0);
    }

    void sample(float samp) 
    {
        SDL_LockAudioDevice(audiodev);
        int plus1 = (this->sndCount + 1) & this->mask;
        if (plus1 == this->sndReadCount) {
            //++this->sndReadCount;
            // generously adjust the buffer in case of overrun
            this->sndReadCount = (this->sndReadCount + (this->mask>>2)) & this->mask;
            ++this->sound_accu_top;
            printf("audio satiated, top=%d\n", this->sound_accu_top);
        }
        this->renderingBuffer[this->sndCount] = samp;
        this->sndCount = plus1;
        SDL_UnlockAudioDevice(audiodev);
    }

#define BIQUAD_FLOAT 1

    void soundStep(int step, int tapeout, int covox, int tapein) 
    {
//        static int int_sound = 0;
//        int newsound = this->timerwrapper.step(step / 2) * 256;
//        newsound += tapeout * 256 - 128;
//        newsound += tapein * 256 - 128;
//        int_sound = (int_sound + newsound) / 2;

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

