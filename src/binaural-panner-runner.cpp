#include "binaural-panner.cpp"
#include "stereo-to-mono.cpp"

int main(){
    std::print("[DEBUG]: HELLO FROM MAIN\n");
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path().parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";
    std::string filter_name = "binaural_rotation";

    // Define filter params
    float rotation_speed = 0.7f; // Radians per second
    float curr_angle = 0.0f;
    bool woodworth_delay = true;
    bool apply_svf = true;
    std::string params_str = std::format("{:.2f}", rotation_speed) + 
                             "_" + std::format("{:.2f}", (float)woodworth_delay) + 
                             "_" + std::format("{:.2f}", (float)apply_svf);

    // Define i.o paths
    fs::path audio_in_path = root_dir / "samples" / audio_name / audio_file_name;
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name / params_str / audio_file_name;

    // Ensure the directory exists before writing
    fs::create_directories(audio_out_path.parent_path());

    AudioFileHandler fh;
    const uint8_t out_channel_count = 2;

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
    BinauralPanner panner;
    StereoToMono mono_conv;

    while ((read_count = fh.read_frames(in_buffer.data(), frame_count)) > 0) {
        std::print("[DEBUG]: Read: {}\n", read_count);

        for (int i = 0; i < read_count; i++) {
            // Assume monaural audio(convert if needed)
            float mono_input = in_buffer[i];
            if (in_channel_count == 2){
                mono_input = mono_conv.process(in_buffer[2*i], in_buffer[2*i+1]);
            }

            // Rotate sound
            panner.process(mono_input, curr_angle, out_buffer[i*2], out_buffer[i*2+1], fh.get_sample_rate(), woodworth_delay, apply_svf);
            curr_angle += (rotation_speed / fh.get_sample_rate());
            if (curr_angle > 2.0f * std::numbers::pi_v<float>)
                curr_angle -= 2.0f * std::numbers::pi_v<float>;
        }

        size_t write_count = fh.write_frames(out_buffer.data(), read_count);
        std::print("[DEBUG]: Write: {}\n", write_count);
    }
}