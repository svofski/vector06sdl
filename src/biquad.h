/*
 * With thanks to Nigel Redmon http://www.earlevel.com/main/2011/01/02/biquad-formulas/
 */
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

class Filter {
public:
    virtual float ffilter(float x) = 0;
    virtual int32_t ifilter(int32_t x) = 0;

    virtual void ba(float b0, float b1, float b2, float a0, float a1) = 0;
    virtual void calcBandpass(int sampleRate, float freq, float Q) = 0;
    virtual void calcLowpass(int sampleRate, float freq, float Q) = 0;
    virtual void calcHighpass(int sampleRate, float freq, float Q) = 0;
    virtual void calcInteger() = 0;

};

class Bypass: public Filter {
public:
    Bypass() {}
    float ffilter(float x) { return x; }
    int32_t ifilter(int32_t x) { return x; }

    void ba(float b0, float b1, float b2, float a0, float a1) {}
    void calcBandpass(int sampleRate, float freq, float Q) {}
    void calcLowpass(int sampleRate, float freq, float Q) {}
    void calcHighpass(int sampleRate, float freq, float Q) {}
    void calcInteger() {}
};

class Biquad: public Filter {
private:
    int m_scale;

public:
    Biquad() {}
    float A0() const { return m_a0; };
    float A1() const { return m_a1; };
    float A2() const { return m_a2; };
    float B1() const { return m_b1; };
    float B2() const { return m_b2; };

    int32_t IA0() const { return m_ia0; };
    int32_t IA1() const { return m_ia1; };
    int32_t IA2() const { return m_ia2; };
    int32_t IB1() const { return m_ib1; };
    int32_t IB2() const { return m_ib2; };

    float ffilter(float x);
    int32_t ifilter(int32_t x);

    int32_t scale() const { return m_scale; }
    int32_t sampleRate() const { return m_SampleRate; };

    float freq() const { return m_Freq; };
    float Q() const { return m_Q; };

    void ba(float b0, float b1, float b2, float a0, float a1);
    void calcBandpass(int sampleRate, float freq, float Q);
    void calcLowpass(int sampleRate, float freq, float Q);
    void calcHighpass(int sampleRate, float freq, float Q);
    void calcInteger();
private:

    float m_Q;
    float m_Freq;
    float m_a0, m_a1, m_a2, m_b1, m_b2;
    float m_x_1, m_x_2, m_y_1, m_y_2;

    int32_t m_SampleRate;

    int32_t m_ia0, m_ia1, m_ia2, m_ib1, m_ib2;
    int32_t m_ix_1, m_ix_2, m_iy_1, m_iy_2;
};


