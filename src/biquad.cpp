#include "biquad.h"

#define FIXP_WBITS     8 

typedef int fixp;
typedef long long       fixpd;
typedef unsigned int fixpu;
typedef unsigned long long fixpud;

#define FIXP_FBITS      (32 - FIXP_WBITS)
#define FIXP_FMASK      (((fixp)1 << FIXP_FBITS) - 1)

#define fixp_rconst(R) ((fixp)((R) * FIXP_ONE + ((R) >= 0 ? 0.5 : -0.5)))
#define fixp_fromint(I) ((fixpd)(I) << FIXP_FBITS)
#define fixp_toint(F) ((F) >> FIXP_FBITS)
#define fixp_add(A,B) ((A) + (B))
#define fixp_sub(A,B) ((A) - (B))
#define fixp_xmul(A,B)                                          \
    ((fixp)(((fixpd)(A) * (fixpd)(B)) >> FIXP_FBITS))
#define fixp_xdiv(A,B)                                          \
    ((fixp)(((fixpd)(A) << FIXP_FBITS) / (fixpd)(B)))
#define fixp_fracpart(A) ((fixp)(A) & FIXP_FMASK)
#define FIXP_ONE        ((fixp)((fixp)1 << FIXP_FBITS))
#define FIXP_ONE_HALF   (FIXP_ONE >> 1)
#define FIXP_TWO        (FIXP_ONE + FIXP_ONE)
#define FIXP_PI         fixp_rconst(3.14159265358979323846)
#define FIXP_TWO_PI     fixp_rconst(2 * 3.14159265358979323846)
#define FIXP_HALF_PI    fixp_rconst(3.14159265358979323846 / 2)
#define FIXP_E          fixp_rconst(2.7182818284590452354)
#define fixp_abs(A) ((A) < 0 ? -(A) : (A))


static inline fixp fixp_mul(fixp A, fixp B)
{
    return (((fixpd)A * (fixpd)B) >> FIXP_FBITS);
}

static inline fixp fixp_div(fixp A, fixp B)
{
    return (((fixpd)A << FIXP_FBITS) / (fixpd)B);
}


void Biquad::ba(float b0, float b1, float b2, float a1, float a2)
{
    m_a0 = b0;
    m_a1 = b1;
    m_a2 = b2;
    m_b1 = a1;
    m_b2 = a2;
    m_scale = 1;

    m_x_2 = 0;
    m_x_1 = 0;
    m_y_2 = 0;
    m_y_1 = 0;
}

void Biquad::calcInteger() {
    m_ia0 = fixp_rconst(m_a0);
    m_ia1 = fixp_rconst(m_a1);
    m_ia2 = fixp_rconst(m_a2);
    m_ib1 = fixp_rconst(m_b1);
    m_ib2 = fixp_rconst(m_b2);
    m_scale = 32768;
}

void Biquad::calcBandpass(int sampleRate, float freq, float Q) {
    float norm;
    float K;

    K = ::tanf(M_PI * freq/sampleRate);
    norm = 1 / (1 + K / Q + K * K);
    m_a0 = K / Q * norm;
    m_a1 = 0;
    m_a2 = -m_a0;
    m_b1 = 2 * (K * K - 1) * norm;
    m_b2 = (1 - K / Q + K * K) * norm;
    m_scale = 1;
}

void Biquad::calcLowpass(int sampleRate, float freq, float Q) {         
    double norm;                                                            
    double K;                                                               

    K = ::tanf(M_PI * freq/sampleRate);
    norm = 1 / (1 + K / Q + K * K);                                         
    m_a0 = K * K * norm;                                                     
    m_a1 = 2 * m_a0;                                                          
    m_a2 = m_a0;                                                              
    m_b1 = 2 * (K * K - 1) * norm;                                           
    m_b2 = (1 - K / Q + K * K) * norm;                                       
    m_scale = 1;
}                                                                       

void Biquad::calcHighpass(int sampleRate, float freq, float Q) {        
    double norm;                                                            
    double K;                                                               

    K = ::tanf(M_PI * freq/sampleRate);                                
    norm = 1 / (1 + K / Q + K * K);                                 
    m_a0 = 1 * norm;                                                         
    m_a1 = -2 * m_a0;                                                         
    m_a2 = m_a0;                                                              
    m_b1 = 2 * (K * K - 1) * norm;                                           
    m_b2 = (1 - K / Q + K * K) * norm;                                       
    m_scale = 1;
}      

float Biquad::ffilter(float x) 
{
    float result = m_a0*x + m_a1*m_x_1 + m_a2*m_x_2 - m_b1*m_y_1 - m_b2*m_y_2;
    m_x_2 = m_x_1;
    m_x_1 = x;
    m_y_2 = m_y_1;
    m_y_1 = result;
    return result;
}

int Biquad::ifilter(int32_t x) {
    fixp q0 = fixp_mul(m_ia0,x); 
    fixp q1 = fixp_mul(m_ia1,m_ix_1);
    fixp q2 = fixp_mul(m_ia2,m_ix_2);

    fixp q3 = fixp_mul(m_ib1,m_iy_1);
    fixp q4 = fixp_mul(m_ib2,m_iy_2);

    int result = (q0 + q1 + q2);
    result = result - (q3 + q4);

    m_ix_2 = m_ix_1;
    m_ix_1 = x;
    m_iy_2 = m_iy_1;
    m_iy_1 = result;
    return result;
}

