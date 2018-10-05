#pragma once

#include <functional>

class Resampler {
public:
    typedef std::function<float(float)> samplefunc;

    Resampler();
    ~Resampler();
    void set_passthrough();
    samplefunc sample;

private:
    void * filter;
    std::function<void(void)> cleanup;

    void create_filter();
};
