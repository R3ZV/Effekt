#include "overdrive.h"

#include <filesystem>
#include <cmath>
#include <vector>

Overdrive::Overdrive(float resistance, float sr, int channels)
    : R_series(resistance), sample_rate(sr), num_channels(channels) {
    a_prev.resize(num_channels, 0.0f);
}

auto Overdrive::apply(const std::vector<float>& input) -> std::vector<float> {
    std::vector<float> output = input;

    for (size_t i = 0; i < output.size(); ++i) {
        int current_channel = i % num_channels;
        float voltageIn = output[i];

        // incident wave
        float a_in = 2.0f * voltageIn - a_prev[current_channel];
        float b_out = scattering(a_in);

        a_prev[current_channel] = b_out;

        // back to voltage
        output[i] = (a_in + b_out) * 0.5f;
    }

    return output;
}

auto Overdrive::scattering(float a_in) -> float {
    // Is: Saturation current
    // Vt: Thermal voltage (~26mV at room temp)
    constexpr float Is = 1e-9f;
    constexpr float Vt = 0.02585f;
    constexpr float R = 10000.0f;

    float v = a_in * 0.5f;
    for (int k = 0; k < 10; ++k) {
        // I = Is * (exp(v/Vt) - 1)
        float current_Is = (v >= 0) ? 1e-9f : 5e-8f;
        float exp_val = std::exp(v / Vt);
        float i_diode = current_Is * (exp_val - 1.0f);

        // Derivative of current w.r.t voltage: g_diode = Is/Vt * exp(v/Vt)
        float g_diode = (current_Is / Vt) * exp_val;

        float f_v = v + R * i_diode - a_in;

        // The derivative: f'(v) = 1 + R * g_diode
        float f_prime = 1.0f + R * g_diode;

        // Newton step
        float delta = f_v / f_prime;
        v -= delta;

        if (std::abs(delta) < 1e-6f) break;
    }

    // WDF Scattering: b = 2*v - a
    return 2.0f * v - a_in;
}

auto Overdrive::get_filter_name() -> std::string { return "overdrive"; }

auto Overdrive::get_output_dir(const std::string& audio_name) -> std::string {
    namespace fs = std::filesystem;
    std::string params_str = "default-params";

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
