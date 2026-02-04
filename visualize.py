import matplotlib.pyplot as plt
from scipy import signal
from scipy.io import wavfile
import numpy as np
import os
from pathlib import Path


# Displays and saves the spectogram
# The output is saved in {spectogram_path}/{title}.png
def draw_spectogram(samples: np.ndarray, sample_rate: int, title: str, save_path: str):
    # Calculate the spectrogram
    frequencies, times, spectrogram = signal.spectrogram(samples, sample_rate)

    # Convert to Decibels: 10 * log10(SXX)
    spectrogram_db = 10 * np.log10(spectrogram + 1e-10)

    # Use pcolormesh with the dB-scaled data
    plt.figure(figsize=(10, 6))
    plt.pcolormesh(times, frequencies, spectrogram_db, shading="auto", cmap="magma")

    plt.ylabel("Frequency [Hz]")
    plt.xlabel("Time [sec]")
    plt.title(title)

    # Add a colorbar to show the dB scale
    plt.colorbar(label="Intensity [dB]")

    # Optional: Limit frequency view if you are looking at specific SVF behavior
    # plt.ylim(0, 20000)
    plt.savefig(f"{save_path}/{title}.png")
    # plt.show()
    plt.close()


# Define paths, files and params
curr_path = Path(os.path.dirname(__file__))

output_path = curr_path / "output"
input_path = curr_path / "input"

# Filter
filter = "moog_ladder"
# resonance = 0.85
# start_cutoff_sweep = 450
# end_cutoff_sweep = 2200
# k_knob = 0.5
rotation_speed = 0.7
woodworth_delay = True
apply_svf = False
params_str = f"{rotation_speed:.2f}_{woodworth_delay:.2f}_{apply_svf:.2f}"
# filter_type = "band_pass"

audio_name = "crawling_scream"
audio_file_name = "audio.wav"

audio_out_dir = output_path / filter / audio_name / params_str
audio_out_file_path = audio_out_dir / audio_file_name

audio_in_dir = input_path / audio_name
audio_in_file_path = audio_in_dir / audio_file_name

# Load the file
sample_rate, samples = wavfile.read("output/envelope_follower/audio.wav")

# Check if the file is integer-based (standard for WAV) and normalize to -1.0 to 1.0
if samples.dtype == np.int16:
    samples = samples.astype(np.float32) / (1 << 15)

# Draw per channel spectogram
if len(samples.shape) > 1:
    sample_count, ch_count = samples.shape
    for i in range(ch_count):
        draw_spectogram(
            samples[:, i], sample_rate, f"{audio_name}_spect_ch_{i}", str("output/envelope_follower/")
        )
else:
    draw_spectogram(samples, sample_rate, f"{audio_name}_mono", str("output/envelope_follower/"))
