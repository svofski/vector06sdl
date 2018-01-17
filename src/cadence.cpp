#include <stdio.h>
#include "cadence.h"

using namespace cadence;
void cadence::set_cadence(int refresh, const int *& cadence, int & length)
{
    static int PULLUP_NONE[] = {1};

    switch (refresh) {
        case 59:
            cadence = calc_pullup<59>();
            length = 59;
            break;
        case 60:
            cadence = calc_pullup<6,5>();
            length = 6;
            break;
        case 70:
            cadence = calc_pullup<7,5>();
            length = 7;
            break;
        case 72:
            cadence = calc_pullup<36,25>();
            length = 36;
            break;
        case 75:
            cadence = calc_pullup<3,2>();
            length = 3;
            break;
        case 85:
            cadence = calc_pullup<17,10>();
            length = 17;
            break;
        case 100:
            cadence = calc_pullup<2,1>();
            length = 2;
            break;
        case 120:
            cadence = calc_pullup<12,5>();
            length = 12;
            break;
        case 144:
            cadence = calc_pullup<144>();
            length = 144;
            break;
        case 160:
            cadence = calc_pullup<16,5>();
            length = 16;
            break;
        case 240:
            cadence = calc_pullup<24,5>();
            length = 24;
            break;
        default:
            printf("Weird refesh rate: %d\n", refresh);
            // fallthrough to 1:1
        case 50:
            cadence = PULLUP_NONE;
            length = 1;
            printf("Cadence will be 1:1\n");
            break;
    }
}
