#include <print>
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <format>
#include <numbers>
#include "AudioFileHandler.cpp"
#include "SVF.cpp"
#include "StereoToMono.cpp"

namespace fs = std::filesystem;

/*OTHER COOL FILTERS: 
- FORMAT (SIMULATE HUMAN SOUND)
- Bitcrusher (LO-FI)
- Envelope follower
- Moog filter
- Binaural stuff
*/

float linear_interpolation(float sample1, float sample2, float sample1_dist){
    return sample1 * (1 - sample1_dist) + sample2 * sample1_dist;
}

// BIG BOOK: Spatial Audio by Francis Rumsey
// Sections: 1.4, 3, 4.7
class BinauralPanner {
private:
    static const int buffer_size = 4410; // 100ms at 44.1kHz
    float delay_buffer_l[buffer_size] = {}, delay_buffer_r[buffer_size] = {};
    int write_index = 0;
    SVF svf_l, svf_r;
public:
    BinauralPanner(){}

    float _get_simple_delay(float angle_rad, uint32_t sample_rate){
        // Calculate ITD based on the sine of the angle
        float max_delay_samples = 0.00065f * sample_rate;
        float itd_offset = std::sin(angle_rad) * max_delay_samples;

        return itd_offset;
    }

    float _get_woodworth_delay(float relative_angle_rad, uint32_t sample_rate) {
        const float c = 340.0f;     // Speed of sound 
        const float r = 0.0875f;    // Average head radius

        float theta = std::abs(relative_angle_rad);

        // Woodworth's Formula
        float itd_seconds = (r * (theta + std::sin(theta))) / c;
        return itd_seconds * (float)sample_rate;
    }

    // Calculates the delayed "write_index" element using interpolation
    float _read_with_interpolation(float* buf, float delay) {
        float read_pos = (float)write_index - delay;
        float wrapped = std::fmod(read_pos, (float)buffer_size);
        
        if (wrapped < 0) 
            wrapped += buffer_size;
        
        int i1 = (int)std::floor(wrapped);
        int i2 = (i1 + 1) % buffer_size;
        return linear_interpolation(buf[i1], buf[i2], wrapped - i1);
    }

    void _apply_svf(float sin_val, float cos_val, float& in_out_sample_l, float& in_out_sample_r) {
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

        in_out_sample_l = svf_l.process(in_out_sample_l, cutoff_l, resonance, low_pass);
        in_out_sample_r = svf_r.process(in_out_sample_r, cutoff_r, resonance, low_pass);
    }

    void _set_delay_buffers(float input){
        delay_buffer_l[write_index] = input;
        delay_buffer_r[write_index] = input;
    }

    void process(float input, float angle_rad, float& out_l, float& out_r, float sample_rate, bool woodworth_delay=true, bool apply_svf=true) {
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
        float gain_l = std::cos(angle_rad * 0.5f + (std::numbers::pi_v<float> * 0.25f));
        float gain_r = std::sin(angle_rad * 0.5f + (std::numbers::pi_v<float> * 0.25f));

        // Set delay buffers
        _set_delay_buffers(input);

        // Read samples for left and right
        out_l = _read_with_interpolation(delay_buffer_l, delay_l);
        out_r = _read_with_interpolation(delay_buffer_r, delay_r);

        // Apply the svf low pass filter on each side
        if (apply_svf)
            _apply_svf(sin_val, cos_val, out_l, out_r);
        
        // Multiply with gain
        out_l *= gain_l;
        out_r *= gain_r;

        write_index = (write_index + 1) % buffer_size;
    }
};