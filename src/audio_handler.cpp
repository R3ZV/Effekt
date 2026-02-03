#include "audio_handler.h"

#include <sndfile.h>

#include <iostream>

auto AudioFileHandler::openRead(const std::string &path) -> bool {
    file_in = sf_open(path.c_str(), SFM_READ, &sfInfo);
    if (!file_in) {
        std::cerr << "Error opening file: " << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }
    return true;
}

auto AudioFileHandler::openWrite(const std::string &path) -> bool {
    file_out = sf_open(path.c_str(), SFM_WRITE, &sfInfo);
    if (!file_out) {
        std::cerr << "Error opening file: " << sf_strerror(nullptr)
                  << std::endl;
        return false;
    }
    return true;
}

auto AudioFileHandler::readFrames(float *buffer, sf_count_t frames)
    -> sf_count_t {
    return sf_readf_float(file_in, buffer, frames);
}

auto AudioFileHandler::writeFrames(float *buffer, sf_count_t frames)
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
