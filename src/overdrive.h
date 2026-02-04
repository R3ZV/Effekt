#pragma once

#include <vector>

#include "amp_filter.h"

class Overdrive : public AMPFilter {
   private:
    float R_series;
    float sample_rate;
    int num_channels;

    std::vector<float> a_prev;

   public:
    Overdrive(float resistance, float sr, int channels = 1);

    auto apply(const std::vector<float>& input) -> std::vector<float> override;

   private:
    auto scattering(float a) -> float;
};
