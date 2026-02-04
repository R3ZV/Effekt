#pragma once

#include <string>
#include <vector>

#include "amp_filter.h"
#include "envelope-follower.h"
#include "svf.h"

class CrybabyEffect : public AMPFilter {
   private:
    uint32_t sample_rate;
    uint8_t ch_count;

    float resonance_start;
    float resonance_factor;
    float sweep_speed_hz;
    float start_cutoff;
    float end_cutoff;
    bool use_env_fol;
    float sweep = 0.0f;

    EnvelopeFollower env_fol_l, env_fol_r;
    SVF svf_left, svf_right;

   public:
    CrybabyEffect(uint8_t ch_count, uint32_t sample_rate,
                  float resonance_start = 0.85, float resonance_factor = 0.12,
                  float sweep_speed_hz = 1.5, float start_cutoff = 450,
                  float end_cutoff = 2500, bool use_env_fol = true)
        : sample_rate(sample_rate),
          ch_count(ch_count),
          resonance_start(resonance_start),
          resonance_factor(resonance_factor),
          sweep_speed_hz(sweep_speed_hz),
          start_cutoff(start_cutoff),
          end_cutoff(end_cutoff),
          use_env_fol(use_env_fol),
          env_fol_l(sample_rate),
          env_fol_r(sample_rate) {};

    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_tri_sweep(float sweep) -> float;
    auto get_cutoff_sweep_exp(float sweep) -> float;
    auto get_resonance_sweep(float sweep) -> float;
    auto get_cutoff(EnvelopeFollower& env_fol, float input, float sweep)
        -> float;
    auto _process(SVF& svf, EnvelopeFollower& env_fol, float input,
                  float tri_sweep, float resonance_sweep) -> float;
    auto get_filter_name() -> std::string override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
};
