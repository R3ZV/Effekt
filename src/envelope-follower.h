#pragma once

class EnvelopeFollower {
   public:
    EnvelopeFollower(float sampleRate);
    void setParams(float attack_ms, float release_ms);
    float process(float input);

   private:
    float fs;
    float envelope;
    float attack_coef;
    float release_coef;
};
