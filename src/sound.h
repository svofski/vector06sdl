#pragma once

#include <atomic>
#include "globaldefs.h"
#include "8253.h"
#include "ay.h"
#include "biquad.h"
#include "SDL.h"
#include "wav.h"

#include "coredsp/filter.h"

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

    Filter * butt1;
    Filter * butt2;

    coredsp::IIR<4, coreutil::simd_t<double>> resample_iir;

    WavRecorder * rec;

public:
    Soundnik(TimerWrapper & tw, AYWrapper & aw) : timerwrapper(tw), 
        aywrapper(aw)
    {}

    void init(WavRecorder * _rec = 0);

    void pause(int pause)
    {
        if (!Options.nosound) {
            SDL_PauseAudioDevice(this->audiodev, pause);
        }
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
            Options.log.audio &&
            fprintf(stderr, "starve rdbuf=%d wrbuf=%d en manque=%d\n", 
                    that->rdbuf, that->wrbuf, that->sound_frame_size - that->wrptr/2);
            that->wrptr = 0;
        } else {
            memcpy(stream, that->buffer[that->rdbuf], len);
            if (++that->rdbuf == Soundnik::NBUFFERS) {
                that->rdbuf = 0;
            }
        }

        that->rec &&
            that->rec->record_buffer(fstream, that->sound_frame_size * 2);

        //if (!Options.vsync) {
            /* sound callback is also our frame interrupt source */
            extern uint32_t timer_callback(uint32_t interval, void * param);
            timer_callback(0, 0);
        //}
    }

    void sample(float samp) 
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

    void soundStep(int step, int tapeout, int covox, int tapein);

};

