#include <iostream>
#include <cmath>
#include <sndfile.h>
#include <vector>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <string>
#include <format> // Required for std::format
#include <numbers>

namespace fs = std::filesystem;

/*OTHER COOL FILTERS: 
- FORMAT (SIMULATE HUMAN SOUND)
- Bitcrusher (LO-FI)
- Envelope follower
- Moog filter
- Binaural stuff
*/
class AudioFileHandler {
private:
    SNDFILE *file_in, *file_out;
    SF_INFO sfInfo;

public:
    AudioFileHandler() : file_in(nullptr), file_out(nullptr) { sfInfo = {0}; }
    ~AudioFileHandler() { close(true); close(false);}

    bool openRead(const std::string& path) {
        file_in = sf_open(path.c_str(), SFM_READ, &sfInfo);
        if (!file_in) {
            std::cerr << "Error opening file: " << sf_strerror(NULL) << std::endl;
            return false;
        }
        return true;
    }

    bool openWrite(const std::string& path) {
        file_out = sf_open(path.c_str(), SFM_WRITE, &sfInfo);
        if (!file_out) {
            std::cerr << "Error opening file: " << sf_strerror(NULL) << std::endl;
            return false;
        }
        return true;
    }

    // Read a block of samples (Interleaved: L, R, L, R...)
    sf_count_t readFrames(float* buffer, sf_count_t frames) {
        return sf_readf_float(file_in, buffer, frames);
    }

    // Write a block of samples
    sf_count_t writeFrames(float* buffer, sf_count_t frames) {
        return sf_writef_float(file_out, buffer, frames);
    }

    int getChannels() const { return sfInfo.channels; }
    int getSampleRate() const { return sfInfo.samplerate; }
    sf_count_t getTotalFrames() const { return sfInfo.frames; }

    void close(bool in_file) {
        if (in_file){
            if (file_in) {
                sf_close(file_in);
                file_in = nullptr;
            }
        }
        else{
            if (file_out) {
                sf_close(file_out);
                file_out = nullptr;
            }
        }
    }
};

enum PassFilterTypes{
    high_pass = 0,
    low_pass = 1,
    band_pass = 2,
    band_shelving = 3,
    notch_filter = 4,
    all_pass_filter = 5,
};

PassFilterTypes from_int(int val){
    return (PassFilterTypes) val;
}

std::string to_string(PassFilterTypes f_type){
    switch (f_type){
        case low_pass:
            return "low_pass";
        case high_pass:
            return "high_pass";
        case band_pass:
            return "band_pass";
        case band_shelving:
            return "band_shelving";
        case notch_filter:
            return "notch_filter";
        case all_pass_filter:
            return "all_pass_filter";
        default:
            return "";
    }
} 

// GOAT: https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf
// SMALLER: https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
class SVF {
private:
    float ic1eq = 0, ic2eq = 0; // The two internal state "integrators"

public:
    // cutoff: 0.0 to 1.0, resonance: 0.0 to 1.0, k_knob: 0.0 to 0.1 (only matters for filter "band_shelving")
    float process(float input, float cutoff, float resonance, PassFilterTypes filter_type, float k_knob = 0) {
        // g is the "elemental" frequency gain
        float gain = tanf(std::numbers::pi_v<float> * (cutoff * 0.5f)); 
        float damping = 2.0f - (2.0f * resonance); // k is the damping factor

        // The math: solving the trapezoidal integration
        float v0 = input;
        float v3 = v0 - ic2eq;
        float v1 = (gain * v3 + ic1eq) / (1.0f + gain * (gain + damping)); // First integral
        float v2 = gain * v1 + ic2eq; // Second integral

        // Update states
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        switch (filter_type){
            case high_pass:
                return v0 - damping * v1 - v2;
            case low_pass:
                return v2;
            case band_pass:
                return v1;
            case band_shelving:
                {// Map 0.0-1.0 to -12dB to +12dB
                float gain_db = (k_knob - 0.5f) * 24.0f; 

                // The book's formula for K
                float K = std::pow(10.0f, gain_db / 20.0f) - 1.0f;
                return v0 + (2.0f * damping * K * v1);
                }
            case notch_filter:
                return v0 - 2.0f * damping * v1;
            case all_pass_filter:
                return v0 - 4.0f * damping * v1;
            default:
                std::cout << "Warning: Unknown filter_type: " << filter_type << std::endl;
                break;
        }
        return 0;
    }
};

int main(){
    std::cout << "[DEBUG]: HELLO FROM MAIN" << std::endl;
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_out_file_name = "audio.wav";
    std::string filter_name = "SVF";

    // Define filter params
    PassFilterTypes filter_type = band_shelving;
    float cutoff = 0.15;
    float resonance = 0.5;
    float k_knob = 0.5; // Even if not used for the other filters, keep it
    std::string params_str = std::format("{:.2f}", cutoff) + "_" +  std::format("{:.2f}", resonance) + "_" + std::format("{:.2f}", k_knob);

    // Define i.o paths
    fs::path audio_in_path = root_dir / "input" / "audio" / (audio_name + ".wav");
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name / params_str / to_string(filter_type) / audio_out_file_name;

    // Ensure the directory exists before writing
    fs::create_directories(audio_out_path.parent_path());

    AudioFileHandler fh;

    // Open input file    
    if (!fh.openRead(audio_in_path.string())){
        std::cout << "[ERROR]: Failed to open file " << audio_in_path << std::endl;
        return -1;
    }

    // Open output file
    if (!fh.openWrite(audio_out_path.string())){
        std::cout << "[ERROR]: Failed to open file " << audio_out_path << std::endl;
        return -1;
    }

    std::cout << fh.getChannels() << std::endl;

    // Define buffer
    size_t frame_count = 4096;
    std::vector<float> buffer(frame_count * fh.getChannels()); 
    size_t read_count = 0;

    // Define filters
    SVF filter_left, filter_right;

    while ((read_count = fh.readFrames(buffer.data(), frame_count)) > 0) {
        int numChannels = fh.getChannels();

        for (int i = 0; i < read_count; ++i) {
            if (numChannels == 2) {
                // Sample L is at index i * 2, Sample R is at index i * 2 + 1
                buffer[i * 2]     = filter_left.process(buffer[i * 2], cutoff, resonance, filter_type, k_knob);
                buffer[i * 2 + 1] = filter_right.process(buffer[i * 2 + 1], cutoff, resonance, filter_type, k_knob);
            } else {
                buffer[i] = filter_right.process(buffer[i], cutoff, resonance, filter_type, k_knob);
            }
        }

        std::cout << "[DEBUG]: Read: " << read_count << std::endl;
        
        size_t write_count = fh.writeFrames(buffer.data(), read_count);

        std::cout << "[DEBUG]: Write: " << write_count << std::endl;
    }
}