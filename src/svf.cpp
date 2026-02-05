#include "svf.h"

#include <sndfile.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

auto SVF::process(float input, float cutoff, float resonance,
                  PassFilterTypes filter_type, float shelving_fact) -> float {
    float gain = tanf(std::numbers::pi_v<float> * (cutoff * 0.5f));
    float damping = 2.0f - (2.0f * resonance);

    // The math: solving the trapezoidal integration
    float v0 = input;
    float v3 = v0 - ic2eq;
    float v1 = (gain * v3 + ic1eq) /
               (1.0f + gain * (gain + damping));  // First integral
    float v2 = gain * v1 + ic2eq;                 // Second integral

    // Update states
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;

    switch (filter_type) {
        case PassFilterTypes::high_pass:
            return v0 - damping * v1 - v2;
        case PassFilterTypes::low_pass:
            return v2;
        case PassFilterTypes::band_pass:
            return v1;
        case PassFilterTypes::band_shelving:
            return v0 + 2.0f * damping * shelving_fact * v1;
        case PassFilterTypes::notch_filter:
            return v0 - 2.0f * damping * v1;  // Just band_shelving with k=1
        case PassFilterTypes::all_pass_filter:
            return v0 - 4.0f * damping * v1;  // Just band_shelving with k=2
        case PassFilterTypes::peaking_filter:
            return std::min(
                1.0f,
                v2 - (v0 - damping * v1 -
                      v2));  // it's just low_pass - high_pass and a wet cut!
        default:
            std::cout << "Warning: Unknown filter_type: "
                      << to_string(filter_type) << std::endl;
            break;
    }
    return 0;
}
