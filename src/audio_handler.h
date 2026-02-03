#pragma once

#include <sndfile.h>

#include <string>

class AudioFileHandler {
   private:
    SNDFILE *file_in, *file_out;
    SF_INFO sfInfo;

   public:
    AudioFileHandler() : file_in(nullptr), file_out(nullptr) { sfInfo = {0}; }
    ~AudioFileHandler() {
        close(true);
        close(false);
    }

    // opens the file to be read from
    auto openRead(const std::string &path) -> bool;

    // opens the file to write to
    auto openWrite(const std::string &path) -> bool;

    // Read a block of samples (Interleaved: L, R, L, R...)
    auto readFrames(float *buffer, sf_count_t frames) -> sf_count_t;

    // Write a block of samples
    auto writeFrames(float *buffer, sf_count_t frames) -> sf_count_t;

    auto getChannels() const -> int { return sfInfo.channels; }
    auto getSampleRate() const -> int { return sfInfo.samplerate; }
    auto getTotalFrames() const -> sf_count_t { return sfInfo.frames; }

    auto close(bool in_file) -> void;
};
