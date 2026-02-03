#include "audio_handler.h"

#include <sndfile.h>

#include <iostream>

auto AudioFileHandler::open_read(const std::string &path) -> bool {
    file_in = sf_open(path.c_str(), SFM_READ, &sf_info_in);
        if (!file_in) {
            std::print(stderr, "Error opening file: {}\n", sf_strerror(NULL));
            return false;
        }
        return true;
}

auto AudioFileHandler::open_write(const std::string& path, uint8_t channel_count = 0)
    -> bool {
        this->sf_info_out = sf_info_in;
        if (channel_count > 0)
            sf_info_out.channels = channel_count;
        file_out = sf_open(path.c_str(), SFM_WRITE, &sf_info_out);
        if (!file_out) {
            std::print(stderr, "Error opening file: {}\n", sf_strerror(NULL));
            return false;
        }
        return true;
}

auto AudioFileHandler::read_frames(float *buffer, sf_count_t frames)
    -> sf_count_t {
    return sf_readf_float(file_in, buffer, frames);
}

auto AudioFileHandler::write_frames(float *buffer, sf_count_t frames)
    -> sf_count_t {
    return sf_writef_float(file_out, buffer, frames);
}

auto AudioFileHandler::close(bool in_file) -> void {
    if (in_file) {
        if (file_in) {
            sf_close(file_in);
            file_in = nullptr;
        }
    } else {
        if (file_out) {
            sf_close(file_out);
            file_out = nullptr;
        }
    }
}
