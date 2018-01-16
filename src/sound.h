#pragma once

#include "8253.h"
#include "ay.h"
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
        this->sound_accu_top = 100 * timer_cycles_per_second / this->sampleRate; // 3210
        this->sound_accu_int = 0;

        printf("SDL audio dev: %d, sample rate=%d "
                "have.samples=%d have.channels=%d have.format=%d have.size=%d\n", 
                this->audiodev, this->sampleRate, 
                have.samples, have.channels, have.format, have.size);
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
            printf("audio starved: diff=%d count=%d\n", diff, count);
        } 
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

        /* sound callback is also our frame interrupt source */
        extern uint32_t timer_callback(uint32_t interval, void * param);
        timer_callback(0, 0);
    }

    void sample(float samp) 
    {
        int plus1 = (this->sndCount + 1) & this->mask;
        if (plus1 == this->sndReadCount) {
            ++this->sndReadCount;
            //printf("AUDIO OVERRUN\n");
        }
        this->renderingBuffer[this->sndCount] = samp;
        this->sndCount = plus1;
    }

    void soundStep(int step, int tapeout, int covox, int tapein) 
    {
        static int int_sound = 0;
        int newsound = this->timerwrapper.step(step / 2) * 256;
        newsound += tapeout * 256 - 128;
        newsound += tapein * 256 - 128;
        int_sound = (int_sound + newsound) / 2;

        //// ay step should execute, but the sound can be sampled only 
        //// when needed, no filtering necessary
        this->aywrapper.step2(step);

        // it's okay if sound is not used this time, the state is kept in the filters
        //sound = this->butt2.filter(this->butt1.filter(sound));

        this->sound_accu_int += 100;
        if (this->sound_accu_int >= this->sound_accu_top) {
            this->sound_accu_int -= this->sound_accu_top;
            float sound = (int_sound + covox)/1024.0f;
            sound += this->aywrapper.value() - 0.5f;
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

