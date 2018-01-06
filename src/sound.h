#pragma once

#include "8253.h"
#include "SDL.h"

class Soundnik
{
private:
    TimerWrapper & timerwrapper;
    //AYWrapper & aywrapper;
    SDL_AudioDeviceID audiodev;
    bool mute;

    static const int renderingBufferSize = 8192;
    static const int sdlBufferSize = 2048;
    float renderingBuffer[renderingBufferSize];
    static const int mask = renderingBufferSize - 1;;
    int sndCount;
    int sndReadCount;
    int sampleRate;

    float soundAccu, soundRatio;

public:
    Soundnik(TimerWrapper & tw) : timerwrapper(tw), mute(false)
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
        };

        this->sampleRate = have.freq;
        // one second = 50 frames
        // raster time in 12 MHz pixelclocks = 768 columns by 312 lines
        // timer clocks = pixel clock / 8
        this->soundRatio = this->sampleRate / (float)(50 * 768 * 312 / 8); 
        //this->soundRatio *= 1.16;

        printf("SDL audio dev: %d, sample rate=%d, soundRatio=%f "
                "have.samples=%d have.channels=%d have.format=%d have.size=%d\n", 
                this->audiodev, this->sampleRate, this->soundRatio,
                have.samples, have.channels, have.format, have.size);

    }

    void pause(int pause)
    {
        SDL_PauseAudioDevice(this->audiodev, pause);
        this->soundAccu = this->sndReadCount = this->sndCount = 0;
    }

    static void callback(void * userdata, uint8_t * stream, int len)
    {
        Soundnik * that = (Soundnik *)userdata;
        int diff = (that->sndCount - that->sndReadCount) & that->mask;
        float * fs = (float *)stream;
        int src = that->sndReadCount;
        int count = len / sizeof(float) / 2;
        if (diff < count) {
            printf("audio starved: diff=%d count=%d\n", diff, count);
        } 
        else {
            printf("audio ok: diff=%d count=%d\n", diff, count);
        }
        int end = count;
        if (diff < end) end = diff;
        int i, dst;
        for (i = 0, dst = 0; i < end; ++i) {
            fs[dst++] = that->renderingBuffer[src]; // Left
            fs[dst++] = that->renderingBuffer[src]; // Right
            src = (src + 1) & that->mask;
        }
        for (; i < count; ++i) {
            fs[dst++] = 0;
            fs[dst++] = 0;
        }
        that->sndReadCount = src;
    }

    void sample(float samp) 
    {
        int plus1 = (this->sndCount + 1) & this->mask;
        if (plus1 == this->sndReadCount) {
            ++this->sndReadCount;
            printf("AUDIO OVERRUN\n");
        }
        //if (plus1 != this->sndReadCount) {
            this->renderingBuffer[this->sndCount] = samp;
            this->sndCount = plus1;
        //} else {
        //}
    }

    void soundStep(int step, int tapeout, int covox) 
    {
        static float sound = 0;
        
        float newsound = this->timerwrapper.step(step / 2);
        newsound += tapeout - 0.5;

        sound = (sound + newsound) / 2;

        //// ay step should execute, but the sound can be sampled only 
        //// when needed, no filtering necessary
        //this->aywrapper.step2(step);

        // it's okay if sound is not used this time, the state is kept in the filters
        //sound = this->butt2.filter(this->butt1.filter(sound));

        this->soundAccu += this->soundRatio;
        if (this->soundAccu >= 1.0) {
            this->soundAccu -= 1.0;
            sound += covox / 256;
            //sound += this->aywrapper.last - 0.5;
            sound = (sound - 1.5) * 0.3;
            if (sound > 1.0) { 
                sound = 1.0; 
            } else if (sound < -1.0) { 
                sound = -1.0; 
            }
            this->sample(sound);
        }
    }

};

