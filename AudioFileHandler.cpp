#include <sndfile.h>
#include <string>
#include <print>
class AudioFileHandler {
private:
    SNDFILE *file_in, *file_out;
    SF_INFO sf_info_in, sf_info_out;

public:
    AudioFileHandler() : file_in(nullptr), file_out(nullptr) { sf_info_in = {0}; sf_info_out = {0};}
    ~AudioFileHandler() { close(true); close(false);}

    bool open_read(const std::string& path) {
        file_in = sf_open(path.c_str(), SFM_READ, &sf_info_in);
        sf_info_out = sf_info_in;
        if (!file_in) {
            std::print(stderr, "Error opening file: {}\n", sf_strerror(NULL));
            return false;
        }
        return true;
    }

    bool open_write(const std::string& path) {
        file_out = sf_open(path.c_str(), SFM_WRITE, &sf_info_out);
        if (!file_out) {
            std::print(stderr, "Error opening file: {}\n", sf_strerror(NULL));
            return false;
        }
        return true;
    }

    // Read a block of samples (Interleaved: L, R, L, R...)
    sf_count_t read_frames(float* buffer, sf_count_t frames) {
        return sf_readf_float(file_in, buffer, frames);
    }

    // Write a block of samples
    sf_count_t write_frames(float* buffer, sf_count_t frames) {
        return sf_writef_float(file_out, buffer, frames);
    }

    int get_channels() const { return sf_info_in.channels; }
    int get_sample_rate() const { return sf_info_in.samplerate; }
    sf_count_t get_total_frames() const { return sf_info_in.frames; }

    void close(bool in_file) {
        if (in_file){
            if (file_in) {
                sf_close(file_in);
                file_in = nullptr;
            }
        }
        else{
            if (file_out) {
                sf_close(file_out);
                file_out = nullptr;
            }
        }
    }
};
