#include <print>
#include <cmath>
#include <vector>
#include <filesystem>
#include <format>
#include "AudioFileHandler.cpp"
#include "SVF.cpp"

/*RELEVANT PARAMS FOR CRYBABY: 
resonance_start
start_cutoff_sweep
end_cutoff_sweep
sweep_speed_hz // One full sweep every 0.66 seconds
resonance_mult 
*/

// YOU CAN ALSO PLAY WITH THE FUNCTION THAT SETS 'cutoff_sweep', IT CURRENTLY AN EXPONENTIAL

// Makes the sweep triangular
float get_tri_sweep(float sweep){
    return 0.5f * (1.0f - cosf(2.0f * std::numbers::pi_v<float> * sweep));
}

// Maps 'sweep' (which is in (0,1)) to the range ('start_cutoff', 'end_cutoff')
// And normalizes the result to be between (0, 1) using 'sample_rate'
float get_cutoff_sweep_exp(float start_cutoff, float end_cutoff, uint32_t sample_rate, float sweep){
    return (start_cutoff * pow(end_cutoff / start_cutoff, sweep)) / sample_rate;
}

// Returns the resonance sweep capped at 0.99
float get_resonance_sweep(float resonance_start, float resonance_mult, float sweep){
    return std::min(0.99f, resonance_start + (resonance_mult * sweep));
}

namespace fs = std::filesystem;
int main(){
    std::print("[DEBUG]: HELLO FROM MAIN\n");
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";
    std::string filter_name = "SVF";

    // Define input path
    fs::path audio_in_path = root_dir / "input" / audio_name / audio_file_name;

    AudioFileHandler fh;

    // Open input file    
    if (!fh.open_read(audio_in_path.string())){
        std::print("[ERROR]: Failed to open file {}\n", audio_in_path.string());
        return -1;
    }

    uint8_t channel_count = fh.get_channels();
    uint64_t total_frame_count = fh.get_total_frames();
    uint32_t sample_rate = fh.get_sample_rate();

    std::print("[DEBUG]: Channel count: {}\n", channel_count);
    std::print("[DEBUG]: Total frame count: {}\n", total_frame_count);
    std::print("[DEBUG]: Sample rate: {}\n", sample_rate);

    // Define filter params
    PassFilterTypes filter_type = band_pass;
    float resonance_start = 0.85;
    float start_cutoff_sweep = 450.0;
    float end_cutoff_sweep = 2200.0;
    float k_knob = 0.5; // Even if not used for the other filters, keep it
    std::string params_str = std::format("{:.2f}", resonance_start) + "_" + std::format("{:.2f}", k_knob) + 
                             "_" + std::format("{:.2f}", start_cutoff_sweep) + "_" + std::format("{:.2f}", end_cutoff_sweep);
    
    // Define and create output path
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name / params_str / to_string(filter_type) / audio_file_name;
    fs::create_directories(audio_out_path.parent_path());
    
    // Open output file
    if (!fh.open_write(audio_out_path.string())){
        std::print("[ERROR]: Failed to open file {}\n", audio_out_path.string());
        return -1;
    }

    // Define buffer
    size_t frame_count = 4096;
    std::vector<float> buffer(frame_count * channel_count); 
    size_t read_count = 0;

    // Define filters
    SVF filter_left, filter_right;

    // Define sweep
    float sweep = 0.0f;
    float sweep_speed_hz = 1.5f; // One full sweep every 0.66 seconds
    float resonance_mult = 0.12f;

    while ((read_count = fh.read_frames(buffer.data(), frame_count)) > 0) {
        std::print("[DEBUG]: Read: {}\n", read_count);

        for (int i = 0; i < read_count; ++i) {
            // Update sweep
            sweep += sweep_speed_hz / sample_rate;
            if (sweep > 1.0f) 
                sweep -= 1.0f;

            // Convert sweep to a triangle wave (0.0 to 1.0 and back)
            float tri_sweep = get_tri_sweep(sweep);

            // Sweep cutoff and resonance
            float cutoff_sweep = get_cutoff_sweep_exp(start_cutoff_sweep, end_cutoff_sweep, sample_rate, tri_sweep);
            float resonance_sweep = get_resonance_sweep(resonance_start, resonance_mult, tri_sweep);
            
            if (channel_count == 2) {
                // Sample L is at index i * 2, Sample R is at index i * 2 + 1
                buffer[i * 2]     = filter_left.process(buffer[i * 2], cutoff_sweep, resonance_sweep, filter_type, k_knob);
                buffer[i * 2 + 1] = filter_right.process(buffer[i * 2 + 1], cutoff_sweep, resonance_sweep, filter_type, k_knob);
            } else {
                buffer[i] = filter_right.process(buffer[i], cutoff_sweep, resonance_sweep, filter_type, k_knob);
            }
        }
        
        size_t write_count = fh.write_frames(buffer.data(), read_count);
        std::print("[DEBUG]: Write: {}\n", write_count);
    }
}