#include "binaural_panner.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>

// BIG BOOK: Spatial Audio by Francis Rumsey

BinauralPanner::BinauralPanner(uint8_t ch_count, uint32_t sample_rate,
                               bool woodworth_delay, bool apply_svf,
                               float rotation_speed)
    : ch_count(ch_count),
      sample_rate(sample_rate),
      woodworth_delay(woodworth_delay),
      apply_svf(apply_svf),
      rotation_speed(rotation_speed) {}

auto BinauralPanner::_get_simple_delay(float angle_rad, uint32_t sample_rate)
    -> float {
    // Calculate ITD based on the sine of the angle
    float max_delay_samples = 0.00065f * sample_rate;
    float itd_offset = std::sin(angle_rad) * max_delay_samples;

    return itd_offset;
}

auto BinauralPanner::_get_woodworth_delay(float relative_angle_rad,
                                          uint32_t sample_rate) -> float {
    const float c = 340.0f;   // Speed of sound
    const float r = 0.0875f;  // Average head radius

    float theta = std::abs(relative_angle_rad);

    // Woodworth's Formula
    float itd_seconds = (r * (theta + std::sin(theta))) / c;
    return itd_seconds * (float)sample_rate;
}

// Calculates the delayed "write_index" element using interpolation
auto BinauralPanner::_read_with_interpolation(float* buf, float delay)
    -> float {
    float read_pos = (float)write_index - delay;
    float wrapped = std::fmod(read_pos, (float)buffer_size);

    if (wrapped < 0) wrapped += buffer_size;

    int i1 = (int)std::floor(wrapped);
    int i2 = (i1 + 1) % buffer_size;
    return std::lerp(buf[i1], buf[i2], wrapped - i1);
}

auto BinauralPanner::_apply_svf(float sin_val, float cos_val,
                                float& in_out_sample_l, float& in_out_sample_r)
    -> void {
    // Calculate a Front-Back factor (0.0 = front, 1.0 = back)
    float back_factor = std::clamp((1.0f - cos_val) * 0.5f, 0.0f, 1.0f);

    // Calculate Side factors (0.0 = near, 1.0 = far/shadowed)
    // For Left ear, sin_val = 1 is "far" (right side).
    float shadow_l = std::clamp(sin_val, 0.0f, 1.0f);
    // For Right ear, sin_val = -1 is "far" (left side).
    float shadow_r = std::clamp(-sin_val, 0.0f, 1.0f);

    // Define cutoffs and resonance
    float normal_cutoff = 0.99f;
    float shadow_cutoff = 0.35f;
    float rear_cutoff = 0.20f;
    float resonance = 0.5;

    // Smoothly interpolate the cutoffs
    float cutoff_l = std::lerp(normal_cutoff, shadow_cutoff, shadow_l);
    cutoff_l = std::lerp(cutoff_l, rear_cutoff, back_factor);

    float cutoff_r = std::lerp(normal_cutoff, shadow_cutoff, shadow_r);
    cutoff_r = std::lerp(cutoff_r, rear_cutoff, back_factor);

    in_out_sample_l = svf_l.process(in_out_sample_l, cutoff_l, resonance,
                                    PassFilterTypes::low_pass);
    in_out_sample_r = svf_r.process(in_out_sample_r, cutoff_r, resonance,
                                    PassFilterTypes::low_pass);
}

auto BinauralPanner::_set_delay_buffers(float input) -> void {
    delay_buffer_l[write_index] = input;
    delay_buffer_r[write_index] = input;
}

auto BinauralPanner::_process(float input, float angle_rad, float& out_l,
                              float& out_r, float sample_rate,
                              bool woodworth_delay, bool apply_svf) -> void {
    float cos_val = std::cos(angle_rad);
    float sin_val = std::sin(angle_rad);

    // Get the absolute shortest angle to the median plane (0 is front/back)
    float azimuth_inc = std::abs(std::atan2(sin_val, std::abs(cos_val)));

    // Determine hemisphere for Delay assignment
    bool is_on_right = sin_val > 0;

    // Calculate delay in samples
    float itd_samples;
    if (woodworth_delay)
        itd_samples = _get_woodworth_delay(azimuth_inc, sample_rate);
    else
        itd_samples = _get_simple_delay(angle_rad, sample_rate);

    float delay_l = is_on_right ? itd_samples : 0.0f;
    float delay_r = is_on_right ? 0.0f : itd_samples;

    // Calculate ILD Gains
    float gain_l =
        std::cos(angle_rad * 0.5f + (std::numbers::pi_v<float> * 0.25f));
    float gain_r =
        std::sin(angle_rad * 0.5f + (std::numbers::pi_v<float> * 0.25f));

    // Set delay buffers
    _set_delay_buffers(input);

    // Read samples for left and right
    out_l = _read_with_interpolation(delay_buffer_l, delay_l);
    out_r = _read_with_interpolation(delay_buffer_r, delay_r);

    // Apply the svf low pass filter on each side
    if (apply_svf) _apply_svf(sin_val, cos_val, out_l, out_r);

    // Multiply with gain
    out_l *= gain_l;
    out_r *= gain_r;

    write_index = (write_index + 1) % buffer_size;
}

auto BinauralPanner::apply(const std::vector<float>& input)
    -> std::vector<float> {
    uint32_t frame_count = input.size() / ch_count;
    std::vector<float> output(frame_count * 2);

    for (int i = 0; i < frame_count; i++) {
        // Assume monaural audio(convert if needed)
        float mono_input = input[i];
        if (ch_count == 2) {
            mono_input = mono_conv.process(input[2 * i], input[2 * i + 1]);
        }

        // Rotate sound
        _process(mono_input, curr_angle, output[i * 2], output[i * 2 + 1],
                 sample_rate, woodworth_delay, apply_svf);
        curr_angle += (rotation_speed / sample_rate);
        if (curr_angle > 2.0f * std::numbers::pi_v<float>)
            curr_angle -= 2.0f * std::numbers::pi_v<float>;
    }

    return output;
}

auto BinauralPanner::get_filter_name() -> std::string {
    return "binaural_rotation";
}

auto BinauralPanner::get_output_dir(const std::string& audio_name)
    -> std::string {
    namespace fs = std::filesystem;
    std::string params_str =
        std::format("{:.2f}_{:.2f}_{:.2f}", rotation_speed,
                    (float)woodworth_delay, (float)apply_svf);

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
