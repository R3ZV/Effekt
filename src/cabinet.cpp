#include "cabinet.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "audio_handler.h"
#include "fft.h"
#include "sndfile.h"

CabinetConvolver::CabinetConvolver(const std::string& ir_path, int block_size)
    : block_size(block_size) {
    AudioFileHandler ir_handler;
    if (!ir_handler.openRead(ir_path)) {
        throw std::runtime_error("CabinetConvolver: Failed to load IR: " +
                                 ir_path);
    }

    sf_count_t total_frames = ir_handler.getTotalFrames();
    int channels = ir_handler.getChannels();
    std::vector<float> buffer(total_frames * channels);

    sf_count_t frames_read = ir_handler.readFrames(buffer.data(), total_frames);
    if (frames_read < total_frames) {
        std::cout << "[WARNING] File shorter than expected. Read: "
                  << frames_read << " frames." << std::endl;
        total_frames = frames_read;
    }

    std::vector<float> ir_mono;
    ir_mono.reserve(total_frames);
    if (channels > 1) {
        for (sf_count_t i = 0; i < total_frames; ++i) {
            ir_mono.push_back(buffer[i * channels]);
        }
    } else {
        ir_mono = std::move(buffer);
    }

    init_partitions(ir_mono);

    std::cout << "[INFO]: Cabinet Loaded. Size: " << ir_mono.size()
              << " samples. Partitions: " << ir_partitions_freq.size()
              << std::endl;
}

auto CabinetConvolver::init_partitions(const std::vector<float>& ir) -> void {
    fft_size = block_size * 2;

    size_t num_partitions = (ir.size() + block_size - 1) / block_size;
    ir_partitions_freq.resize(num_partitions);

    for (size_t p = 0; p < num_partitions; ++p) {
        std::vector<Complex> padded_chunk(fft_size, {0.0f, 0.0f});

        size_t start_idx = p * block_size;
        size_t end_idx = std::min(start_idx + block_size, ir.size());

        for (size_t i = start_idx; i < end_idx; ++i) {
            padded_chunk[i - start_idx] = {ir[i], 0.0f};
        }

        ir_partitions_freq[p] = FFT::compute(padded_chunk, FFT_DIR::forward);
    }

    overlap_buffer.assign(block_size, 0.0f);
    fdl.resize(num_partitions, std::vector<Complex>(fft_size, {0.0f, 0.0f}));
}

auto CabinetConvolver::apply(const std::vector<float>& input)
    -> std::vector<float> {
    if (input.empty()) return input;

    std::vector<Complex> padded_input(fft_size, {0.0f, 0.0f});
    size_t n_copy = std::min((size_t)block_size, input.size());
    for (size_t i = 0; i < n_copy; ++i) {
        padded_input[i] = {input[i], 0.0f};
    }

    std::vector<Complex> current_input_fft =
        FFT::compute(padded_input, FFT_DIR::forward);

    fdl.push_front(current_input_fft);
    fdl.pop_back();

    std::vector<Complex> fft_accumulator(fft_size, {0.0f, 0.0f});

    for (size_t p = 0; p < ir_partitions_freq.size(); ++p) {
        for (int k = 0; k < fft_size; ++k) {
            fft_accumulator[k] += fdl[p][k] * ir_partitions_freq[p][k];
        }
    }

    auto time_domain_complex = FFT::compute(fft_accumulator, FFT_DIR::backward);

    std::vector<float> output(block_size);
    float normalizer = 1.0f / fft_size;

    for (int i = 0; i < block_size; ++i) {
        output[i] =
            (time_domain_complex[i].real() * normalizer) + overlap_buffer[i];

        overlap_buffer[i] =
            time_domain_complex[i + block_size].real() * normalizer;
    }

    return output;
}
