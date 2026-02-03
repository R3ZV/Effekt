#include <print>
#include <string>
#include <cmath>
#include <numbers>

/*OTHER COOL FILTERS: 
- FORMAT (SIMULATE HUMAN SOUND)
- Bitcrusher (LO-FI)
- Envelope follower
- Moog filter
- Binaural stuff
*/

enum PassFilterTypes{
    high_pass = 0,
    low_pass = 1,
    band_pass = 2,
    band_shelving = 3,
    notch_filter = 4,
    all_pass_filter = 5,
    peaking_filter = 6
};

PassFilterTypes from_int(int val){
    return (PassFilterTypes) val;
}

std::string to_string(PassFilterTypes f_type){
    switch (f_type){
        case low_pass:
            return "low_pass";
        case high_pass:
            return "high_pass";
        case band_pass:
            return "band_pass";
        case band_shelving:
            return "band_shelving";
        case notch_filter:
            return "notch_filter";
        case all_pass_filter:
            return "all_pass_filter";
        case peaking_filter:
            return "peaking_filter";
        default:
            return "";
    }
} 

// GOAT: https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf
// SMALLER: https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
class SVF {
private:
    float ic1eq = 0, ic2eq = 0; // The two internal state "integrators"

public:
    // cutoff: 0.0 to 1.0, resonance: 0.0 to 1.0, k_knob: 0.0 to 0.1 (only matters for filter "band_shelving")
    float process(float input, float cutoff, float resonance, PassFilterTypes filter_type, float shelving_fact = 0) {
        // g is the "elemental" frequency gain
        float gain = tanf(std::numbers::pi_v<float> * (cutoff * 0.5f)); 
        float damping = 2.0f - (2.0f * resonance); // k is the damping factor

        // The math: solving the trapezoidal integration
        float v0 = input;
        float v3 = v0 - ic2eq;
        float v1 = (gain * v3 + ic1eq) / (1.0f + gain * (gain + damping)); // First integral
        float v2 = gain * v1 + ic2eq; // Second integral

        // Update states
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        switch (filter_type){
            case high_pass:
                return v0 - damping * v1 - v2;
            case low_pass:
                return v2;
            case band_pass:
                return v1;
            case band_shelving:
                return v0 + 2.0f * damping * shelving_fact * v1;
            case notch_filter:
                return v0 - 2.0f * damping * v1; // Just band_shelving with k=1
            case all_pass_filter:
                return v0 - 4.0f * damping * v1; // Just band_shelving with k=2
            case peaking_filter:
                return std::min(1.0f, v2 - (v0 - damping * v1 - v2)); // it's just low_pass - high_pass and a wet cut!
            default:
                std::print("Warning: Unknown filter_type: {}\n", static_cast<int>(filter_type));
                break;
        }
        return 0;
    }
};