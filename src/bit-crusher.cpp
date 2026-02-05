#include "bit-crusher.h"
#include <cmath>
#include <numbers>
#include <filesystem>
#include <format>

float Bitcrusher::_process(float input, uint8_t bits, float downsample) {
    sample_counter += 1.0f;
    float wet = 0.5;
    
    // Only update the 'last_sample' when the counter hits the threshold
    if (sample_counter >= downsample) {
        // Calculate how many discrete levels we have (e.g., 8-bit = 256 levels)
        float levels = std::pow(2.0f, bits);
        
        // Crush signal
        last_sample = std::round(input * levels) / levels;
        
        // Reset counter (accounting for fractional downsample values)
        sample_counter -= downsample;
    }

    return std::lerp(input, last_sample, wet);
}

auto BitcrusherFilter::apply(const std::vector<float>& input)
    -> std::vector<float> {
    uint32_t frame_count = input.size() / ch_count;
    std::vector<float> output(input.size());
    
    for (int i = 0; i < frame_count; ++i) {
        if (ch_count == 2) {
            output[2 * i] = bc_left._process(input[2 * i], max_bits, max_downsample);
            output[2 * i + 1] = bc_right._process(input[2 * i + 1], max_bits, max_downsample);
        } else {
            output[i] = bc_left._process(input[2 * i], max_bits, max_downsample);
        }
    }

    return output;
}

auto BitcrusherFilter::get_filter_name() -> std::string {
    return "bitcrusher";
}

auto BitcrusherFilter::get_output_dir(const std::string& audio_name)
    -> std::string {
    namespace fs = std::filesystem;
    std::string params_str = std::format("{:.2f}_{:.2f}", (float)max_bits, max_downsample);

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
