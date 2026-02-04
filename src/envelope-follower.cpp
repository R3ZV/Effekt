#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <vector>
#include "audio_handler.h"
#include <print>
#include <filesystem>
#include <cmath>

namespace fs = std::filesystem;

// Reference implementation: https://www.musicdsp.org/en/latest/Analysis/97-envelope-detector.html
// https://www.uncini.com/dida/tsa/mod_tsa/Chap_08.pdf
// 8.7.2: Dynamic Gain Control, 8.7.3: Signal level calculation
class EnvelopeFollower {
private:
    float fs;
    float envelope;
    float attack_coef;
    float release_coef;
public:
    EnvelopeFollower(float sampleRate) : fs(sampleRate) {
        envelope = 0.0f;
        setParams(10.0f, 100.0f);
    }

    // attack_ms:  How fast to track a volume spike
    // release_ms: How fast to drop when silence hits 
    void setParams(float attack_ms, float release_ms) {
        // Convert milliseconds to seconds
        float att_sec = attack_ms / 1000.0f;
        float rel_sec = release_ms / 1000.0f;

        // Calculate 1-pole coefficients
        // Formula: coeff = exp(-1 / (time * sampleRate))
        if (att_sec > 0.0f)
            attack_coef = std::exp(-1.0f / (att_sec * fs));
        else
            attack_coef = 0.0f; // Instant attack

        if (rel_sec > 0.0f)
            release_coef = std::exp(-1.0f / (rel_sec * fs));
        else
            release_coef = 0.0f; // Instant release
    }

    float process(float input) {
        float absInput = std::abs(input);

        // If the signal is higher than current envelope -> Attack Phase
        if (absInput > envelope) {
            // Attack: Move envelope towards input
            envelope = attack_coef * envelope + (1.0f - attack_coef) * absInput;
        } 
        else { // If the signal is lower than current envelope -> Release Phase
            envelope = release_coef * envelope + (1.0f - release_coef) * absInput;
        }

        return envelope;
    }
};