#include "resampler.h"
#include "coredsp/filter.h"

using namespace std;

struct FilterContainer
{
    coredsp::IIR<6, coreutil::simd_t<double>> * filter;

    FilterContainer() 
    {
        // pretty open still, but can't close more: gets unstable
        // [b,a] = cheby2(5,62,10000/1.5e6*2*3.131);
        float cheby2b[] = {
            0.0002431447672463337285157780609523570092278532683849334716796875,
            -0.0007128289616887240835035877140057891665492206811904907226562500,
            0.0004699117780832894161920088027528663587872870266437530517578125,
            0.0004699117780832894161920088027528663587872870266437530517578125,
            -0.0007128289616887240835035877140057891665492206811904907226562500,
            0.0002431447672463337014107237488147461590415332466363906860351562,
        };

        float cheby2a[] = {
            1.0000000000000000000000000000000000000000000000000000000000000000,
            -4.8253882333290443185092044586781412363052368164062500000000000000,
            9.3167053593195738869781052926555275917053222656250000000000000000,
            -8.9969767456033036268081559683196246623992919921875000000000000000,
            4.3454171650598842902013529965188354253768920898437500000000000000,
            -0.8397570902798295877644818574481178075075149536132812500000000000,
        };
        filter = new coredsp::IIR<6, coreutil::simd_t<double>>();
        filter->reset();
        filter->coefs(cheby2b, cheby2a);
    }

    ~FilterContainer()
    {
        delete filter;
    }

};

Resampler::Resampler() 
{
    FilterContainer * fc = new FilterContainer();
    this->filtercontainer = fc;

    process = [fc](float s) { return fc->filter->tick(s); };
}

Resampler::~Resampler()
{
    delete (FilterContainer *)this->filtercontainer;
}

float Resampler::sample(float s) 
{
    return process(s);
}

void Resampler::set_passthrough() 
{
    process = [] (float s) { return s; };
}
