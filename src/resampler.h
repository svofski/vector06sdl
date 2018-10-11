#pragma once

class Resampler {
public:
    Resampler();
    ~Resampler();
    void set_passthrough(bool thru);
    float sample(float s);

    bool egg;

private:
    void create_filter();

private:
    static constexpr int nlevels = 1;

    float interp_buf[128];
    int iidx;
    int dcm_ctr;

    void *f[nlevels];
    int ctr[nlevels];
    float in[nlevels + 1];
    bool thru;
};
