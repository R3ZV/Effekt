#include "pitch_shifter.h"

#include <filesystem>
#include <cmath>
#include <vector>

PitchShifter::PitchShifter(float pitch_factor, float sample_rate, int channels)
    : pitch_factor(pitch_factor), channels(channels), cursor(0), phasor(0.0f) {
    // used to reduce the buzz
    grain_frames = static_cast<size_t>(0.050f * sample_rate);

    buff_size = grain_frames * 4;
    buffer.resize(buff_size * channels, 0.0f);
}

auto PitchShifter::apply(const std::vector<float>& input)
    -> std::vector<float> {
    std::vector<float> output(input.size());
    size_t frames = input.size() / channels;

    // rate at which the delay time changes.
    // if pitch == 1.0, rate is 0 (delay is constant).
    // if pitch == 2.0, rate is negative (we catch up to the write head).
    double phase_increment = (1.0f - pitch_factor) / grain_frames;

    for (size_t i = 0; i < frames; ++i) {
        for (int c = 0; c < channels; ++c) {
            buffer[cursor * channels + c] = input[i * channels + c];
        }

        double phasor_a = phasor;
        double phasor_b = phasor + 0.5f;
        if (phasor_b >= 1.0f) phasor_b -= 1.0f;

        // how far back in the buffer to read
        auto delay_a = static_cast<float>(phasor_a * grain_frames);
        auto delay_b = static_cast<float>(phasor_b * grain_frames);

        for (int c = 0; c < channels; ++c) {
            float sample_a = get_sample(cursor, delay_a, c);
            float sample_b = get_sample(cursor, delay_b, c);

            // triangle window: 1.0 at center (0.5), 0.0 at edges (0.0 and 1.0)
            float weight_a = 1.0f - 2.0f * std::abs((float)phasor_a - 0.5f);
            float weight_b = 1.0f - 2.0f * std::abs((float)phasor_b - 0.5f);

            output[i * channels + c] =
                (sample_a * weight_a) + (sample_b * weight_b);
        }

        cursor = (cursor + 1) % buff_size;
        phasor += phase_increment;

        // keep phasor in [0.0, 1.0) range
        if (phasor >= 1.0f) phasor -= 1.0f;
        if (phasor < 0.0f) phasor += 1.0f;
    }

    return output;
}

auto PitchShifter::get_sample(size_t current_pos, float delay, int channel)
    -> float {
    float read_idx = static_cast<float>(current_pos) - delay;
    while (read_idx < 0.0f) read_idx += buff_size;
    while (read_idx >= buff_size) read_idx -= buff_size;

    auto i1 = static_cast<size_t>(read_idx);
    size_t i2 = (i1 + 1) % buff_size;
    float frac = read_idx - i1;

    float s1 = buffer[i1 * channels + channel];
    float s2 = buffer[i2 * channels + channel];

    return s1 + frac * (s2 - s1);
}


auto PitchShifter::get_filter_name() -> std::string { return "pitchshifter"; }

auto PitchShifter::get_output_dir(const std::string& audio_name) -> std::string {
    namespace fs = std::filesystem;
    std::string params_str = "default-params";

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
