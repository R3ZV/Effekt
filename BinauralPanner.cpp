#include <print>
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <format>
#include <numbers>
#include "AudioFileHandler.cpp"

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
public:
    BinauralPanner(){}

    void process(float input, float angle_rad, float& out_l, float& out_r, float sample_rate) {
        // Calculate ILD (Gain)
        float gain_l = std::cos(angle_rad * 0.5f); 
        float gain_r = std::sin(angle_rad * 0.5f);

        // Clamping to absolute values to ensure smoothness
        gain_l = std::max(0.0f, std::abs(gain_l));
        gain_r = std::max(0.0f, std::abs(gain_r));

        // Calculate ITD (Delay in samples)
        // Max delay is ~0.66ms -> ~29 samples at 44.1kHz
        float max_delay_samples = 0.00066f * sample_rate;
        float delay_l = (angle_rad > 0) ? (sin(angle_rad) * max_delay_samples) : 0;
        float delay_r = (angle_rad < 0) ? (fabs(sin(angle_rad)) * max_delay_samples) : 0;

        // Write to Buffers
        delay_buffer_l[write_index] = input;
        delay_buffer_r[write_index] = input;

        // Read with Linear Interpolation (Simplified here as integer)
        float read_l = write_index - delay_l + buffer_size, read_r = write_index - delay_r + buffer_size;
        int read_l_floor = read_l, read_r_floor = read_r;
        float read_l_ceil = read_l - read_l_floor, read_r_ceil = read_r - read_r_floor;

        out_l = linear_interpolation(delay_buffer_l[read_l_floor % buffer_size], delay_buffer_l[(read_l_floor + 1) % buffer_size], read_l_ceil);
        out_r = linear_interpolation(delay_buffer_r[read_r_floor % buffer_size], delay_buffer_r[(read_r_floor + 1) % buffer_size], read_r_ceil);

        write_index = (write_index + 1) % buffer_size;
    }
};

int main(){
    std::print("[DEBUG]: HELLO FROM MAIN\n");
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";
    std::string filter_name = "binaural_rotation";

    // Define filter params
    float rotation_speed = 0.7f; // Radians per second
    float curr_angle = 0.0f;
    std::string params_str = std::format("{:.2f}", rotation_speed) + "_" + std::format("{:.2f}", curr_angle);

    // Define i.o paths
    fs::path audio_in_path = root_dir / "input" / audio_name / audio_file_name;
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name / params_str / audio_file_name;

    // Ensure the directory exists before writing
    fs::create_directories(audio_out_path.parent_path());

    AudioFileHandler fh;

    // Open input file    
    if (!fh.open_read(audio_in_path.string())){
        std::print("[ERROR]: Failed to open file {}\n", audio_in_path.string());
        return -1;
    }

    // Open output file
    if (!fh.open_write(audio_out_path.string())){
        std::print("[ERROR]: Failed to open file {}\n", audio_out_path.string());
        return -1;
    }

    std::print("[DEBUG]: Channel count: {}\n", fh.get_channels());
    
    // Define buffer
    size_t frame_count = 4096;
    std::vector<float> buffer(frame_count * fh.get_channels()); 
    size_t read_count = 0;

    // Define filters
    BinauralPanner panner;

    while ((read_count = fh.read_frames(buffer.data(), frame_count)) > 0) {
        std::print("[DEBUG]: Read: {}\n", read_count);

        for (int i = 0; i < read_count; ++i) {
            float mono_input = buffer[i*2]; // Assume mono input for spatialization
            panner.process(mono_input, curr_angle, buffer[i*2], buffer[i*2+1], fh.get_sample_rate());
            
            curr_angle += (rotation_speed / fh.get_sample_rate());
            if (curr_angle > 2.0f * std::numbers::pi_v<float>) 
                curr_angle -= 2.0f * std::numbers::pi_v<float>;
        }

        size_t write_count = fh.write_frames(buffer.data(), read_count);
        std::print("[DEBUG]: Write: {}\n", write_count);
    }
}