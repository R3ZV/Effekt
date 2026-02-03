#pragma once

#include <complex>
#include <deque>
#include <string>
#include <vector>

#include "amp_filter.h"

// Assuming Complex and FFT types are defined elsewhere as per your code
using Complex = std::complex<float>;

class CabinetConvolver : public AMPFilter {
   public:
    CabinetConvolver(const std::string& ir_path, int block_size);
    auto apply(const std::vector<float>& input) -> std::vector<float> override;

   private:
    auto init_partitions(const std::vector<float>& ir_mono) -> void;

    int block_size;
    int fft_size;

    // Store FFTs of the IR chunks.
    std::vector<std::vector<Complex>> ir_partitions_freq;

    // frequency domain delay line
    std::deque<std::vector<Complex>> fdl;

    std::vector<float> overlap_buffer;
};
