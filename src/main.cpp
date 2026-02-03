#include <sndfile.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "amp_filter.h"
#include "audio_handler.h"
#include "cabinet.h"
#include "svf.h"

namespace fs = std::filesystem;

// TODO: delete after we add SVF as an effect in the effects array
auto old_main() -> int {
    std::cout << "[DEBUG]: HELLO FROM MAIN" << std::endl;
    // Define root path
    std::string curr_file = __FILE_NAME__;
    fs::path root_dir = fs::path(__FILE__).parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";
    std::string filter_name = "SVF";

    // Define filter params
    PassFilterTypes filter_type = PassFilterTypes::all_pass_filter;
    float resonance = 0.99;
    float start_cutoff_sweep = 0.15;
    float end_cutoff_sweep = 0.15;
    float k_knob = 0.5;  // Even if not used for the other filters, keep it
    std::string params_str =
        std::format("{:.2f}_{:.2f}_{:.2f}_{:.2f}", resonance, k_knob,
                    start_cutoff_sweep, end_cutoff_sweep);

    // Define i.o paths
    fs::path audio_in_path = root_dir / "input" / audio_name / audio_file_name;
    fs::path audio_out_path = root_dir / "output" / filter_name / audio_name /
                              params_str / to_string(filter_type) /
                              audio_file_name;

    // Ensure the directory exists before writing
    fs::create_directories(audio_out_path.parent_path());

    AudioFileHandler fh;

    // Open input file
    if (!fh.open_read(audio_in_path.string())) {
        std::cout << "[ERROR]: Failed to open file " << audio_in_path
                  << std::endl;
        return -1;
    }

    // Open output file
    if (!fh.open_write(audio_out_path.string())) {
        std::cout << "[ERROR]: Failed to open file " << audio_out_path
                  << std::endl;
        return -1;
    }

    std::cout << fh.get_channels() << std::endl;

    // Define buffer
    size_t frame_count = 4096;
    std::vector<float> buffer(frame_count * fh.get_channels());
    size_t read_count = 0;

    // Define filters
    SVF filter_left, filter_right;

    while ((read_count = fh.read_frames(buffer.data(), frame_count)) > 0) {
        int numChannels = fh.get_channels();

        float cutoff_sweep = start_cutoff_sweep;
        float cutoff_sweep_step =
            (end_cutoff_sweep - start_cutoff_sweep) / read_count;
        for (int i = 0; i < read_count; ++i) {
            if (numChannels == 2) {
                // Sample L is at index i * 2, Sample R is at index i * 2 + 1
                buffer[i * 2] =
                    filter_left.process(buffer[i * 2], cutoff_sweep, resonance,
                                        filter_type, k_knob);
                buffer[i * 2 + 1] =
                    filter_right.process(buffer[i * 2 + 1], cutoff_sweep,
                                         resonance, filter_type, k_knob);
            } else {
                buffer[i] = filter_right.process(
                    buffer[i], cutoff_sweep, resonance, filter_type, k_knob);
            }
            cutoff_sweep += cutoff_sweep_step;
        }

        std::cout << "[DEBUG]: Read: " << read_count << std::endl;

        size_t write_count = fh.write_frames(buffer.data(), read_count);

        std::cout << "[DEBUG]: Write: " << write_count << std::endl;
    }
    return 0;
}

auto main() -> int {
    fs::path input_file = "../samples/calm_guitar/audio.wav";
    fs::path output_file = "../output/audio.wav";
    fs::create_directories(output_file.parent_path());

    AudioFileHandler fh;

    if (!fh.open_read(input_file.string())) {
        std::cerr << "[ERROR]: Failed to open file " << input_file << std::endl;
        return EXIT_FAILURE;
    }

    if (!fh.open_write(output_file.string())) {
        std::cerr << "[ERROR]: Failed to open file " << output_file
                  << std::endl;
        return EXIT_FAILURE;
    }

    const size_t FRAMES_COUNT = 4096;
    std::vector<AMPFilter*> filters;
    AMPFilter* cab = new CabinetConvolver("../samples/ir.wav", FRAMES_COUNT);
    filters.push_back(cab);

    int channels = fh.get_channels();
    std::cout << "[DBG]: Audio file has: " << channels << " channels."
              << std::endl;

    size_t read_count = 0;
    std::vector<float> input_buffer(FRAMES_COUNT * channels);
    while ((read_count = fh.read_frames(input_buffer.data(), FRAMES_COUNT)) >
           0) {
        std::cout << "[DBG]: Read: " << read_count << std::endl;

        std::vector<float> process_buffer(
            input_buffer.begin(),
            input_buffer.begin() + (read_count * fh.get_channels()));

        for (auto filter : filters) {
            process_buffer = filter->apply(process_buffer);
        }

        size_t write_count = fh.write_frames(process_buffer.data(), read_count);

        std::cout << "[DBG]: Wrote: " << write_count << std::endl;
    }

    return EXIT_SUCCESS;
}
