#include "crybaby.h"

#include <cmath>
#include <filesystem>
#include <format>
#include <numbers>

auto CrybabyEffect::get_tri_sweep(float sweep) -> float {
    return 0.5f * (1.0f - cosf(2.0f * std::numbers::pi_v<float> * sweep));
}

auto CrybabyEffect::get_cutoff_sweep_exp(float sweep) -> float {
    return (start_cutoff * pow(end_cutoff / start_cutoff, sweep)) / sample_rate;
}

auto CrybabyEffect::get_resonance_sweep(float sweep) -> float {
    return std::min(0.99f, resonance_start + (resonance_factor * sweep));
}

auto CrybabyEffect::get_cutoff(EnvelopeFollower& env_fol, float input,
                               float sweep) -> float {
    if (use_env_fol)
        return env_fol.process(input);
    else
        return get_cutoff_sweep_exp(sweep);
}

auto CrybabyEffect::_process(SVF& svf, EnvelopeFollower& env_fol, float input,
                             float tri_sweep, float resonance_sweep) -> float {
    float cutoff = get_cutoff(env_fol, input, tri_sweep);
    float result =
        svf.process(input, cutoff, resonance_sweep, PassFilterTypes::band_pass);
    return std::lerp(input, result, 0.8f);
}

auto CrybabyEffect::apply(const std::vector<float>& input)
    -> std::vector<float> {
    uint32_t frame_count = input.size() / ch_count;
    std::vector<float> output(input.size());
    for (int i = 0; i < frame_count; ++i) {
        // Update sweep
        sweep += sweep_speed_hz / sample_rate;
        if (sweep > 1.0f) sweep -= 1.0f;

        // Convert sweep to a triangle wave (0.0 to 1.0 and back)
        float tri_sweep = get_tri_sweep(sweep);

        // Sweep cutoff and resonance
        float resonance_sweep = get_resonance_sweep(tri_sweep);

        if (ch_count == 2) {
            output[2 * i] = _process(svf_left, env_fol_l, input[2 * i],
                                     tri_sweep, resonance_sweep);
            output[2 * i + 1] = _process(svf_right, env_fol_r, input[2 * i + 1],
                                         tri_sweep, resonance_sweep);
        } else {
            output[i] = _process(svf_left, env_fol_l, input[i], tri_sweep,
                                 resonance_sweep);
        }
    }

    return output;
}

auto CrybabyEffect::get_filter_name() -> std::string { return "crybaby"; }

auto CrybabyEffect::get_output_dir(const std::string& audio_name)
    -> std::string {
    namespace fs = std::filesystem;

    std::string params_str =
        std::format("{:.2f}_{:.2f}_{:.2f}_{:.2f}_{:.2f}_{:.2f}",
                    resonance_start, resonance_factor, sweep_speed_hz,
                    (float)use_env_fol, start_cutoff, end_cutoff);

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
