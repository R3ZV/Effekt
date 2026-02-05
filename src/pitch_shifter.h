#pragma once

#include <vector>

#include "amp_filter.h"

class PitchShifter : public AMPFilter {
   public:
    PitchShifter(float pitch_factor, float sample_rate, int channels);
    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
    auto get_filter_name() -> std::string override;

   private:
    auto get_sample(size_t current_pos, float delay, int channel) -> float;

    float pitch_factor;
    int channels;
    size_t grain_frames;
    size_t buff_size;

    std::vector<float> buffer;
    size_t cursor;
    double phasor;
};
