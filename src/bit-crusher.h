#include <vector>
#include <cstdint>
#include <string>
#include "amp_filter.h"

class Bitcrusher{
private:
    float last_sample = 0.0f;
    float sample_counter = 0.0f;
public:
    auto _process(float input, uint8_t bits, float downsample) -> float;
};

class BitcrusherFilter : public AMPFilter{
private:
    uint8_t ch_count, max_bits, curr_bits;
    float max_downsample;
    Bitcrusher bc_left, bc_right;
public:
    BitcrusherFilter(uint8_t ch_count, uint8_t max_bits = 12, float max_downsample = 8.0f)
        : ch_count(ch_count), max_bits(max_bits), max_downsample(max_downsample){}

    auto apply(const std::vector<float>& input) -> std::vector<float> override;
    auto get_output_dir(const std::string& audio_name) -> std::string override;
    auto get_filter_name() -> std::string override;
};