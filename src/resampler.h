#pragma once

class Resampler {
public:
    Resampler();
    ~Resampler();
    void set_passthrough(bool thru);
    float sample(float s);

private:
    void create_filter();

private:
    static constexpr int nlevels = 6;
    void *f[nlevels];
    int ctr[nlevels];
    float out;
    bool thru;
};
