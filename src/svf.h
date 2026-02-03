#pragma once

#include <sndfile.h>

#include <string>

enum class PassFilterTypes {
    high_pass = 0,
    low_pass = 1,
    band_pass = 2,
    band_shelving = 3,
    notch_filter = 4,
    all_pass_filter = 5,
    peaking_filter = 6
};

inline auto from_int(int val) -> PassFilterTypes {
    return (PassFilterTypes)val;
}

inline auto to_string(PassFilterTypes f_type) -> std::string {
    switch (f_type) {
        case PassFilterTypes::low_pass:
            return "low_pass";
        case PassFilterTypes::high_pass:
            return "high_pass";
        case PassFilterTypes::band_pass:
            return "band_pass";
        case PassFilterTypes::band_shelving:
            return "band_shelving";
        case PassFilterTypes::notch_filter:
            return "notch_filter";
        case PassFilterTypes::all_pass_filter:
            return "all_pass_filter";
        case PassFilterTypes::peaking_filter:
            return "peaking_filter";
        default:
            return "";
    }
}

// GOAT:
// https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf
// SMALLER: https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
class SVF {
   private:
    float ic1eq = 0, ic2eq = 0;  // The two internal state "integrators"

   public:
    // cutoff: 0.0 to 1.0, resonance: 0.0 to 1.0, k_knob: 0.0 to 0.1 (only
    // matters for filter "band_shelving")
    auto process(float input, float cutoff, float resonance,
                 PassFilterTypes filter_type, float shelving_fact = 0) -> float;
};
