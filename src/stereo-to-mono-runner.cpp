#include "AudioFileHandler.cpp"
#include "StereoToMono.cpp"
#include <filesystem>
#include <format>
#include <print>
#include <vector>
namespace fs = std::filesystem;

int main(){
    std::print("[DEBUG]: HELLO FROM MAIN\n");
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";
    std::string filter_name = "stereo_to_mono";
    std::string params_str = "";
    // Define i.o paths
    fs::path audio_in_path = root_dir / "input" / audio_name / audio_file_name;
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name / params_str / audio_file_name;

    // Ensure the directory exists before writing
    fs::create_directories(audio_out_path.parent_path());

    AudioFileHandler fh;
    const uint8_t out_channel_count = 1;

    // Open input file    
    if (!fh.open_read(audio_in_path.string())){
        std::print("[ERROR]: Failed to open file {}\n", audio_in_path.string());
        return -1;
    }

    // Open output file
    if (!fh.open_write(audio_out_path.string(), out_channel_count)){
        std::print("[ERROR]: Failed to open file {}\n", audio_out_path.string());
        return -1;
    }

    uint8_t in_channel_count = fh.get_channels();
    uint64_t total_frame_count = fh.get_total_frames();
    uint32_t sample_rate = fh.get_sample_rate();

    std::print("[DEBUG]: Input channel count: {}\n", in_channel_count);
    std::print("[DEBUG]: Input total frame count: {}\n", total_frame_count);
    std::print("[DEBUG]: Input sample rate: {}\n", sample_rate);

    // Define buffers
    size_t frame_count = 4096;
    std::vector<float> in_buffer(frame_count * in_channel_count);
    std::vector<float> out_buffer(frame_count * out_channel_count); 
    size_t read_count = 0;

    // Define filters
    StereoToMono mono_conv;

    while ((read_count = fh.read_frames(in_buffer.data(), frame_count)) > 0) {
        std::print("[DEBUG]: Read: {}\n", read_count);

        for (int i = 0; i < read_count; i++) {
            float mono_input = in_buffer[i];
            if (in_channel_count == 2){ // Apply "Pan Law"
                mono_input = mono_conv.process(in_buffer[2*i], in_buffer[2*i+1]);
            }
            out_buffer[i] = mono_input;
        }

        size_t write_count = fh.write_frames(out_buffer.data(), read_count);
        std::print("[DEBUG]: Write: {}\n", write_count);
    }
}