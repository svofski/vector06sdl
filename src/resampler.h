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

public:
    static constexpr int UP = 5;
    static constexpr int DOWN = 156;

private:

    int dcm_ctr;
    int phase;

    void *filterbank[UP];

    float out;
    bool thru;
};
