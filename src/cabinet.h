#pragma once

#include <complex>
#include <string>
#include <vector>

#include "amp_filter.h"

using Complex = std::complex<float>;

class CabinetConvolver : public AMPFilter {
   public:
    CabinetConvolver(const std::string& ir_path, int block_size);
    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
    auto get_filter_name() -> std::string override;

   private:
    int block_size;
    int fft_size;

    std::vector<Complex> ir_fft_l, ir_fft_r;
    std::vector<float> tail_l, tail_r;
};
