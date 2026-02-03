#include <fftw3.h>

#include <algorithm>
#include <complex>
#include <vector>

enum class FFT_DIR { forward, backward };

class FFT {
   public:
    static auto compute(const std::vector<std::complex<float>>& input,
                        FFT_DIR dir) -> std::vector<std::complex<float>> {
        int nfft = input.size();
        if (nfft == 0) return {};

        // 1. Allocate FFTW-aligned memory
        // We allocate separate buffers using fftwf_malloc to ensure 16-byte
        // alignment required for AVX/SSE optimizations.
        auto* in_raw =
            (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * nfft);
        auto* out_raw =
            (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * nfft);

        // 2. Copy data: std::vector -> FFTW Buffer
        // We cast the FFTW buffer to std::complex* to use std::copy safely.
        // This works because std::complex<float> and fftwf_complex are binary
        // compatible.
        std::copy(input.begin(), input.end(),
                  reinterpret_cast<std::complex<float>*>(in_raw));

        // 3. Create Plan
        // FFTW_FORWARD = -1, FFTW_BACKWARD = +1
        int fftw_dir = (dir == FFT_DIR::forward) ? FFTW_FORWARD : FFTW_BACKWARD;

        // fftwf_plan_dft_1d handles Complex-to-Complex transforms
        fftwf_plan plan =
            fftwf_plan_dft_1d(nfft, in_raw, out_raw, fftw_dir, FFTW_ESTIMATE);

        // 4. Execute
        fftwf_execute(plan);

        // 5. Copy data: FFTW Buffer -> std::vector
        std::vector<std::complex<float>> output(nfft);
        auto* out_ptr = reinterpret_cast<std::complex<float>*>(out_raw);
        output.assign(out_ptr, out_ptr + nfft);

        fftwf_destroy_plan(plan);
        fftwf_free(in_raw);
        fftwf_free(out_raw);

        return output;
    }
};
