#include "cabinet.h"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "audio_handler.h"
#include "fft.h"

CabinetConvolver::CabinetConvolver(const std::string& ir_path, int block_size)
    : block_size(block_size) {
    AudioFileHandler ir_handler;
    if (!ir_handler.open_read(ir_path)) {
        throw std::runtime_error("CabinetConvolver: Failed to load IR: " +
                                 ir_path);
    }

    int num_channels = ir_handler.get_channels();
    const int READ_CHUNK_SIZE = 4096;

    std::vector<float> read_buffer(READ_CHUNK_SIZE * num_channels);
    std::vector<float> ir_raw_interleaved;

    size_t frames_read = 0;
    while ((frames_read = ir_handler.read_frames(read_buffer.data(),
                                                 READ_CHUNK_SIZE)) > 0) {
        ir_raw_interleaved.insert(
            ir_raw_interleaved.end(), read_buffer.begin(),
            read_buffer.begin() + (frames_read * num_channels));
    }

    size_t num_frames = ir_raw_interleaved.size() / num_channels;

    size_t required_size = block_size + num_frames - 1;
    this->fft_size = 1;
    while (this->fft_size < required_size) this->fft_size *= 2;

    std::vector<Complex> ir_padded_L(this->fft_size, {0, 0});
    std::vector<Complex> ir_padded_R(this->fft_size, {0, 0});

    for (size_t i = 0; i < num_frames; ++i) {
        if (num_channels == 1) {
            ir_padded_L[i] = ir_raw_interleaved[i];
            ir_padded_R[i] = ir_raw_interleaved[i];
        } else if (num_channels >= 2) {
            ir_padded_L[i] = ir_raw_interleaved[i * num_channels + 0];
            ir_padded_R[i] = ir_raw_interleaved[i * num_channels + 1];
        }
    }

    int fade_len = std::min((int)num_frames, 256);
    for (int k = 0; k < fade_len; ++k) {
        float gain = 1.0f - ((float)k / (float)fade_len);

        size_t idx = num_frames - 1 - k;

        ir_padded_L[idx] *= gain;
        ir_padded_R[idx] *= gain;
    }

    float max_peak = 0.0f;
    for (size_t i = 0; i < num_frames; ++i) {
        max_peak = std::max(max_peak, std::abs(ir_padded_L[i].real()));
        max_peak = std::max(max_peak, std::abs(ir_padded_R[i].real()));
    }

    if (max_peak > 0.00001f) {
        float scale_factor = 0.5f / max_peak;

        for (size_t i = 0; i < num_frames; ++i) {
            ir_padded_L[i] *= scale_factor;
            ir_padded_R[i] *= scale_factor;
        }
    }

    this->ir_fft_l = FFT::compute(ir_padded_L, FFT_DIR::forward);
    this->ir_fft_r = FFT::compute(ir_padded_R, FFT_DIR::forward);

    this->tail_l.assign(this->fft_size, 0.0f);
    this->tail_r.assign(this->fft_size, 0.0f);
}

auto CabinetConvolver::apply(const std::vector<float>& input)
    -> std::vector<float> {
    if (input.size() > this->block_size * 2) {
        throw std::runtime_error("Input chunk too large.");
    }

    size_t num_frames = input.size() / 2;

    std::vector<Complex> input_L(this->fft_size, {0, 0});
    std::vector<Complex> input_R(this->fft_size, {0, 0});

    for (size_t i = 0; i < num_frames; ++i) {
        input_L[i] = {input[2 * i], 0.0f};
        input_R[i] = {input[2 * i + 1], 0.0f};
    }

    std::vector<Complex> fft_in_L = FFT::compute(input_L, FFT_DIR::forward);
    std::vector<Complex> fft_in_R = FFT::compute(input_R, FFT_DIR::forward);

    auto process_channel =
        [&](const std::vector<Complex>& signal_fft,
            const std::vector<Complex>& ir_fft,
            std::vector<float>& tail_buffer) -> std::vector<float> {
        std::vector<Complex> spectrum(this->fft_size);
        for (size_t k = 0; k < this->fft_size; ++k) {
            spectrum[k] = signal_fft[k] * ir_fft[k];
        }

        std::vector<Complex> time_domain =
            FFT::compute(spectrum, FFT_DIR::backward);

        std::vector<float> output_channel;
        output_channel.reserve(num_frames);

        for (size_t k = 0; k < num_frames; ++k) {
            float sample = time_domain[k].real() / (float)this->fft_size;
            sample += tail_buffer[k];
            output_channel.push_back(sample);
        }

        size_t remaining_tail = this->fft_size - num_frames;
        for (size_t k = 0; k < remaining_tail; ++k) {
            tail_buffer[k] = tail_buffer[k + num_frames];
        }
        std::fill(tail_buffer.begin() + remaining_tail, tail_buffer.end(),
                  0.0f);

        for (size_t k = num_frames; k < this->fft_size; ++k) {
            float val = time_domain[k].real() / (float)this->fft_size;
            tail_buffer[k - num_frames] += val;  // Accumulate!
        }

        return output_channel;
    };

    std::vector<float> out_L =
        process_channel(fft_in_L, this->ir_fft_l, this->tail_l);
    std::vector<float> out_R =
        process_channel(fft_in_R, this->ir_fft_r, this->tail_r);

    std::vector<float> stereo_output;
    stereo_output.reserve(input.size());

    for (size_t i = 0; i < num_frames; ++i) {
        stereo_output.push_back(out_L[i]);
        stereo_output.push_back(out_R[i]);
    }

    return stereo_output;
}

auto CabinetConvolver::get_filter_name() -> std::string {
    return "pitchshifter";
}

auto CabinetConvolver::get_output_dir(const std::string& audio_name)
    -> std::string {
    namespace fs = std::filesystem;
    std::string params_str = "default-params";

    fs::path audio_out_path =
        fs::path(get_filter_name()) / audio_name / params_str;
    return audio_out_path.string();
}
