#pragma once

#include <sndfile.h>

#include <string>

class AudioFileHandler {
   private:
    SNDFILE *file_in, *file_out;
    SF_INFO sf_info_in, sf_info_out;

public:
   public:
    AudioFileHandler() : file_in(nullptr), file_out(nullptr) { sf_info_in = {0}; sf_info_out = {0};}
    ~AudioFileHandler() { close(true); close(false);}

    // opens the file to be read from
    auto open_read(const std::string &path) -> bool;

    // opens the file to write to
    auto open_write(const std::string &path, uint8_t channel_count = 0) -> bool;

    // Read a block of samples (Interleaved: L, R, L, R...)
    auto read_frames(float *buffer, sf_count_t frames) -> sf_count_t;

    // Write a block of samples
    auto write_frames(float *buffer, sf_count_t frames) -> sf_count_t;

    int get_channels() const { return sf_info_in.channels; }
    int get_sample_rate() const { return sf_info_in.samplerate; }
    sf_count_t get_total_frames() const { return sf_info_in.frames; }

    auto close(bool in_file) -> void;
};
