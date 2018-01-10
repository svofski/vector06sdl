#pragma once

#include <inttypes.h>
#include <vector>

using namespace std;

class Wav
{
private:
    vector<int16_t> Data;

public:
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;

private:
    template <typename valtype> 
    typename enable_if<!is_same<valtype,string>::value, valtype>::type
        token(vector<uint8_t> & raw, size_t & offset)
    {
        valtype result = *(valtype *)&raw[offset];
        offset += sizeof(valtype);
        return result;
    }

    template <typename valtype>
    typename enable_if<is_same<valtype,string>::value, valtype>::type
        token(vector<uint8_t> & raw, size_t & offset)
    {
        string result((const char *) &raw[offset], 4); 
        offset += 4;
        return result;
    }

    template <typename srctype, 
                typename enable_if<is_same<srctype, uint8_t>::value, srctype>::type = 0
             >
    void merge_stereo(const void * _src, size_t count)
    {
        const srctype * src = (srctype *) _src;
        for (size_t i = 0, o = 0; i < count;) {
            int y = (src[i++] - 128) * 256;
            if (this->NumChannels == 2) {
                y += (src[i++] - 128) * 256;
                y /= 2;
            }
            this->Data[o++] = y;
        }
    }

    template <typename srctype, 
                typename enable_if<!is_same<srctype, uint8_t>::value, srctype>::type = 0
             >
    void merge_stereo(const void * _src, size_t count)
    {
        const srctype * src = (srctype *) _src;
        for (size_t i = 0, o = 0; i < count;) {
            int y = src[i++];
            if (this->NumChannels == 2) {
                y += src[i++];
                y /= 2;
            }
            this->Data[o++] = y;
        }
    }

public:
    bool set_bytes(vector<uint8_t> & raw)
    {
        size_t sptr = 0;

        string chunkid = token<string>(raw, sptr);

        if (chunkid != "RIFF") {
            printf("Not a RIFF file\n");
            return false;
        }

        uint32_t globsize = token<uint32_t>(raw, sptr);

        string wave = token<string>(raw, sptr);
        if (wave != "WAVE") {
            printf("No WAVE format signature\n");
            return false;
        }

        string fmtid = token<string>(raw, sptr);
        if (fmtid != "fmt ") {
            printf("No fmt subchunk\n");
            return false;
        }

        uint32_t subchunksize = token<uint32_t>(raw, sptr);
        size_t nextchunk = sptr + subchunksize;

        uint16_t audioformat = token<uint16_t>(raw, sptr);
        if (audioformat != 1) {
            printf("Audioformat must be type 1, PCM\n");
            return false;
        }

        this->NumChannels = token<uint16_t>(raw, sptr);
        this->SampleRate =  token<uint32_t>(raw, sptr);
        this->ByteRate =    token<uint32_t>(raw, sptr);
        this->BlockAlign =  token<uint16_t>(raw, sptr);
        this->BitsPerSample=token<uint16_t>(raw, sptr);

        printf("WAV file: Channels: %d Sample rate: %d Byte rate: %d "
                "Block align: %d Bits per sample: %d\n",
                this->NumChannels, this->SampleRate, this->ByteRate, 
                this->BlockAlign, this->BitsPerSample);

        for (;;) {
            sptr = nextchunk;
            string chunk2id = token<string>(raw, sptr);
            uint32_t chunk2sz = token<uint32_t>(raw, sptr);
            nextchunk = sptr + chunk2sz;
            if (chunk2id == "data") {
                switch (this->BitsPerSample) {
                    case 8:
                        this->Data.resize(chunk2sz / this->NumChannels);
                        this->merge_stereo<uint8_t>(&raw[sptr], chunk2sz);
                        break;
                    case 16:
                        this->Data.resize(chunk2sz / 2 / this->NumChannels);
                        this->merge_stereo<int16_t>(&raw[sptr], chunk2sz / 2);
                        break;
                    case 32:
                        this->Data.resize(chunk2sz / 4 / this->NumChannels);
                        this->merge_stereo<int32_t>(&raw[sptr], chunk2sz / 4);
                        break;
                    default:
                        printf("Unsupported bits per sample: %d\n", 
                                this->BitsPerSample);
                        return false;
                        break;
                }
                return true;
            }
        }
        return false;
    }

    int sample_at(size_t pos) const 
    {
        return this->Data[pos];
    }

    int size() const 
    {
        return this->Data.size();
    }
};

class WavPlayer
{
private:
    Wav & wav;
    size_t playhead;
    int ratio;
    int frac;

public:
    WavPlayer(Wav & _wav) : wav(_wav)
    {
    }

    void init()
    {
        this->ratio = 59904 * 50 / wav.SampleRate;
        this->frac = 0;
    }

    void rewind()
    {
        playhead = 0;
    }

    void advance(int instruction_time)
    {
        if (this->playhead < this->wav.size()) {
            if (this->ratio == 0) {
                this->init();
            }
            this->frac += instruction_time;
            if (this->frac > this->ratio) {
                this->frac -= this->ratio;
                ++this->playhead;
            }
        }
    }

    int sample() const
    {
        if (playhead < this->wav.size()) {
            return wav.sample_at(playhead) > 0 ? 1 : 0;
        }
        return 0;
    }
};
