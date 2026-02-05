#include "envelope_follower.h"

#include <cmath>

// Reference implementation:
// https://www.musicdsp.org/en/latest/Analysis/97-envelope-detector.html
// https://www.uncini.com/dida/tsa/mod_tsa/Chap_08.pdf
// 8.7.2: Dynamic Gain Control, 8.7.3: Signal level calculation

EnvelopeFollower::EnvelopeFollower(float sampleRate) : fs(sampleRate) {
    envelope = 0.0f;
    set_params(10.0f, 100.0f);
}

// attack_ms:  How fast to track a volume spike
// release_ms: How fast to drop when silence hits
auto EnvelopeFollower::set_params(float attack_ms, float release_ms) -> void {
    // Convert milliseconds to seconds
    float att_sec = attack_ms / 1000.0f;
    float rel_sec = release_ms / 1000.0f;

    // Calculate 1-pole coefficients
    // Formula: coeff = exp(-1 / (time * sampleRate))
    if (att_sec > 0.0f)
        attack_coef = std::exp(-1.0f / (att_sec * fs));
    else
        attack_coef = 0.0f;  // Instant attack

    if (rel_sec > 0.0f)
        release_coef = std::exp(-1.0f / (rel_sec * fs));
    else
        release_coef = 0.0f;  // Instant release
}

auto EnvelopeFollower::process(float input) -> float {
    float absInput = std::abs(input);

    // If the signal is higher than current envelope -> Attack Phase
    if (absInput > envelope) {
        // Attack: Move envelope towards input
        envelope = attack_coef * envelope + (1.0f - attack_coef) * absInput;
    } else {  // If the signal is lower than current envelope -> Release Phase
        envelope = release_coef * envelope + (1.0f - release_coef) * absInput;
    }

    return envelope;
}
