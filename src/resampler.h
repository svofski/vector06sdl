#pragma once

#include <functional>

class Resampler {
public:
    typedef std::function<float(float)> samplefunc;

    Resampler();
    ~Resampler();
    float sample(float s);
    void set_passthrough();

private:
    samplefunc process;
    void * filtercontainer;
};
