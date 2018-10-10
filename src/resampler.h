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
    static constexpr int nlevels = 5;
    void *f[nlevels];
    int ctr[nlevels];
    float in[nlevels + 1];
    bool thru;
};
