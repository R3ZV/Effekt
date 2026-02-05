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


# Load the file
sample_rate, samples = wavfile.read("output/combination/crawling_scream/audio.wav")

# Check if the file is integer-based (standard for WAV) and normalize to -1.0 to 1.0
if samples.dtype == np.int16:
    samples = samples.astype(np.float32) / (1 << 15)

# Draw per channel spectogram
if len(samples.shape) > 1:
    sample_count, ch_count = samples.shape
    for i in range(ch_count):
        draw_spectogram(
            samples[:, i], sample_rate, f"spect_ch_{i}", "output/combination/crawling_scream/"
        )
else:
    draw_spectogram(samples, sample_rate, f"spect_mono", "output/combination/crawling_scream/")
