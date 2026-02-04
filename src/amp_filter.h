#pragma once

#include <typeinfo>

#include <string>
#include <vector>
class AMPFilter {
   public:
    virtual ~AMPFilter() = default;
    virtual auto apply(const std::vector<float>& input)
        -> std::vector<float> = 0;

    // By default returns "../output/{in_file_name}/audio.wav"
    virtual auto get_output_dir(const std::string& audio_name) -> std::string {
        return "";
    }

    virtual auto get_filter_name() -> std::string {
        return typeid(this).name();
    }
};
