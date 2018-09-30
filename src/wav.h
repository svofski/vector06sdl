#pragma once

#include <inttypes.h>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>

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

    std::function<void(void)> onloaded;

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

        uint32_t globsize = token<uint32_t>(raw, sptr); (void)globsize;

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
                if (this->onloaded) {
                    this->onloaded();
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

    size_t size() const 
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
    bool loaded;

public:
    WavPlayer(Wav & _wav) : wav(_wav)
    {
        WavPlayer & that = *this;
        wav.onloaded = [&that]() {
            that.loaded = true;
        };
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
        if (this->loaded) {
            if(this->playhead < this->wav.size()) {
                if (this->ratio == 0) {
                    this->init();
                }
                this->frac += instruction_time;
                if (this->frac > this->ratio) {
                    this->frac -= this->ratio;
                    ++this->playhead;
                }
            }
            else {
                this->loaded = false;
            }
        }
    }

    int sample() const
    {
        if (this->loaded && playhead < this->wav.size()) {
            return wav.sample_at(playhead) > 0 ? 1 : 0;
        }
        return 0;
    }
};

class WavRecorder 
{
private:
    ofstream file;
    std::vector<int16_t> buffer;
    static const size_t buffer_size = 65536;
    size_t offset = 0;
    size_t length_pos;
    uint32_t data_size;

public:
    WavRecorder()
    {
    }

    void init(const std::string & path)
    {
        file.open(path, ios::out | ios::binary);

        buffer.resize(8192);
        offset = 0;
        data_size = 0;

        static const uint8_t header[] = {
            0x52, 0x49, 0x46, 0x46, 0x24, 0xb0, 0x2b, 
            0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 
            0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x00, 
            0xee, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 
            0x00, 0xb0, 0x2b, 0x00, };

        file.write((char *)header, sizeof(header));
        length_pos = sizeof(header)/sizeof(header[0]) - 4;
    }

    int record_sample(float left, float right)
    {
        buffer[offset++] = (int) (left * 32767 + 0.5);
        buffer[offset++] = (int) (right * 32767 + 0.5);
        if (offset >= buffer.size()) {
            file.write((const char *)buffer.data(), sizeof(uint16_t) * buffer.size());
            offset = 0;
        }
        data_size += 4;
        return 0;
    }

    int record_buffer(const float * fstream, const size_t count)
    {
        for (size_t i = 0; i < count; i += 2) {
            record_sample(fstream[i], fstream[i+1]);
        }
        return count;
    }

    ~WavRecorder()
    {
        if (file.is_open()) {
            if (offset > 0) {
                file.write((const char *)buffer.data(), sizeof(uint16_t) * offset);
            }
            file.seekp(length_pos);
            file.write((const char *)&data_size, 4);
            printf("~WavRecorder: data size=%x\n", data_size);
        }
    }
};
