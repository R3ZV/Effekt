#include <sndfile.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <vector>

#include "amp_filter.h"
#include "audio_handler.h"
#include "binaural_panner.h"
#include "cabinet.h"
#include "crybaby.h"
#include "overdrive.h"
#include "svf.h"
#include "bit-crusher.h"

namespace fs = std::filesystem;

auto main() -> int {
    fs::path root_dir = fs::path(__FILE__).parent_path().parent_path();

    // Define params for i/o paths
    std::string audio_name = "crawling_scream";
    std::string audio_file_name = "audio.wav";

    // Define i.o paths
    fs::path audio_in_path =
        root_dir / "samples" / audio_name / audio_file_name;

    AudioFileHandler fh;
    const uint8_t out_channel_count = 2;

    if (!fh.open_read(audio_in_path.string())) {
        std::print("[ERROR]: Failed to open file {}\n", audio_in_path.string());
        return -1;
    }

    int channels = fh.get_channels();
    float sample_rate = fh.get_sample_rate();

    // Define output file and create output directory
    fs::path audio_out_path =
        root_dir / "output" / "combination"/ audio_name / audio_file_name;
    fs::create_directories(audio_out_path.parent_path());

    if (!fh.open_write(audio_out_path.string(), out_channel_count)) {
        std::print("[ERROR]: Failed to open file {}\n",
                   audio_out_path.string());
        return -1;
    }
    std::print("[DEBUG]: File opened: {}\n", audio_out_path.string());

    const size_t FRAMES_COUNT = 4096;
    std::vector<AMPFilter*> filters;

    filters.push_back(new CrybabyEffect(channels, sample_rate));
    filters.push_back(new Overdrive(1000.0f, sample_rate, channels));
    filters.push_back(bitcrusher); // Not a grea addition :)))
    filters.push_back(new CabinetConvolver("../samples/ir.wav", FRAMES_COUNT));
    filters.push_back(new BinauralPanner(channels, sample_rate));

    size_t read_count = 0;
    std::vector<float> input_buffer(FRAMES_COUNT * channels);
    while ((read_count = fh.read_frames(input_buffer.data(), FRAMES_COUNT)) >
           0) {
        std::print("[DBG]: Read: {}\n", read_count);

        std::vector<float> process_buffer(
            input_buffer.begin(),
            input_buffer.begin() + (read_count * channels));

        for (auto filter : filters) {
            process_buffer = filter->apply(process_buffer);
        }

        size_t write_count = fh.write_frames(process_buffer.data(), read_count);

        std::print("[DBG]: Wrote: {}\n", write_count);
    }

    for (auto f : filters) delete f;

    return EXIT_SUCCESS;
}
