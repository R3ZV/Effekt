#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "amp_filter.h"
#include "stereo_to_mono.cpp"
#include "svf.h"

class BinauralPanner : public AMPFilter {
   public:
    BinauralPanner(uint8_t ch_count, uint32_t sample_rate,
                   bool woodworth_delay = true, bool apply_svf = true,
                   float rotation_speed = 0.7);

    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
    auto get_filter_name() -> std::string override;

   private:
    bool woodworth_delay, apply_svf;
    float rotation_speed;
    uint8_t ch_count;
    uint32_t sample_rate;
    float curr_angle = 0;

    static const int buffer_size = 4410;
    float delay_buffer_l[buffer_size] = {}, delay_buffer_r[buffer_size] = {};
    int write_index = 0;
    SVF svf_l, svf_r;

    StereoToMono mono_conv;

    auto _get_simple_delay(float angle_rad, uint32_t sample_rate) -> float;
    auto _get_woodworth_delay(float relative_angle_rad, uint32_t sample_rate)
        -> float;
    auto _read_with_interpolation(float* buf, float delay) -> float;
    void _apply_svf(float sin_val, float cos_val, float& in_out_sample_l,
                    float& in_out_sample_r);
    void _set_delay_buffers(float input);
    void _process(float input, float angle_rad, float& out_l, float& out_r,
                  float sample_rate, bool woodworth_delay = true,
                  bool apply_svf = true);
};
