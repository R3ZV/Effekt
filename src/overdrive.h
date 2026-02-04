#pragma once

#include <vector>

#include "amp_filter.h"

class Overdrive : public AMPFilter {
   public:
    Overdrive(float resistance, float sr, int channels = 1);

    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
    auto get_filter_name() -> std::string override;

   private:
    auto scattering(float a) -> float;

    float R_series;
    float sample_rate;
    int num_channels;

    std::vector<float> a_prev;
};
