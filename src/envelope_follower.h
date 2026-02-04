#pragma once

class EnvelopeFollower {
   public:
    EnvelopeFollower(float sampleRate);
    auto set_params(float attack_ms, float release_ms) -> void;
    auto process(float input) -> float;

   private:
    float fs;
    float envelope;
    float attack_coef;
    float release_coef;
};
