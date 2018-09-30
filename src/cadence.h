#pragma once

namespace cadence {

template <unsigned N, unsigned M = 50> int * calc_pullup()
{
    static int cadence[N];
    double err, de;
    de = (double)M / N;
    err = de;
    int on = 0;
    for (size_t i = 0; i < N; ++i) {
        err += de;
        cadence[i] = err > 1;
        if (err > 1) {
            err -= 1.0;
            ++on;
        }
        printf("%d ", cadence[i]);
    }
    printf(": %d/%d\n", on, N);
    return cadence;
}

void set_cadence(int refresh, const int *& cadence, int & length);

}
