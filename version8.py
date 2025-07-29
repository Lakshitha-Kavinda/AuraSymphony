import numpy as np
import pyaudio
import socket
import time
import sys
from scipy import signal

# === CONFIG ===
NUM_BANDS = 18
CHUNK = 1024
RATE = 44100
UDP_IP = "192.168.167.85"  # Replace with your ESP32's IP address
UDP_PORT = 21324
FREQ_MIN = 50  # Hz, for sub-bass
FREQ_MAX = 2700  # Hz, for musical harmonics
SMOOTHING_FACTOR = 0.3  # Lower for faster response
NOISE_THRESHOLD = 5000  # Minimum FFT amplitude to consider
MIN_ACTIVATION_THRESHOLD = 60000  # Minimum amplitude for LED activation
MAX_AMPLITUDE = 100000  # Expected max FFT amplitude for loud music

# === INIT ===
def initialize_audio():
    try:
        p = pyaudio.PyAudio()
        stream = p.open(
            format=pyaudio.paInt16,
            channels=1,
            rate=RATE,
            input=True,
            frames_per_buffer=CHUNK
        )
        return p, stream
    except Exception as e:
        print(f"Audio initialization failed: {e}")
        sys.exit(1)

def initialize_udp():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        print(f"Socket:{sock}")
        return sock
    except socket.error as e:
        print(f"UDP initialization failed: {e}")
        sys.exit(1)

# Logarithmic frequency bins with validation
def get_frequency_bins():
    freqs = np.logspace(np.log10(FREQ_MIN), np.log10(FREQ_MAX), NUM_BANDS + 1)
    bin_edges = (freqs * CHUNK // RATE).astype(int)
    bin_edges = np.clip(bin_edges, 0, CHUNK // 2)
    for i in range(1, len(bin_edges)):
        if bin_edges[i] <= bin_edges[i-1]:
            bin_edges[i] = bin_edges[i-1] + 1
    return bin_edges

def main():
    p, stream = initialize_audio()
    sock = initialize_udp()
    bin_edges = get_frequency_bins()
    prev_levels = np.zeros(NUM_BANDS)

    # Bandpass filter
    sos = signal.butter(4, [FREQ_MIN, FREQ_MAX], btype='band', fs=RATE, output='sos')

    print("Streaming audio to ESP32 for music concert... Press Ctrl+C to stop.")
    # print("Frequency bins:", bin_edges)

    try:
        while True:
            try:
                data = np.frombuffer(stream.read(CHUNK, exception_on_overflow=False), dtype=np.int16)
                data = signal.sosfilt(sos, data)
                window = np.hanning(len(data))
                data = data * window
                fft = np.abs(np.fft.rfft(data))[:CHUNK // 2]

                # Calculate band amplitudes
                bands = []
                for i in range(NUM_BANDS):
                    start = max(0, bin_edges[i])
                    end = min(len(fft), bin_edges[i+1])
                    if end > start:
                        band = np.mean(fft[start:end])
                    else:
                        band = 0.0
                    bands.append(band if np.isfinite(band) else 0.0)

                # Apply noise threshold
                bands = np.array(bands)
                if np.max(bands) < NOISE_THRESHOLD:
                    bands = np.zeros_like(bands)

                # Apply activation threshold and square-root scaling
                bands = np.where(bands > MIN_ACTIVATION_THRESHOLD, bands, 0)
                bands = np.sqrt(bands)  # Square-root for less amplification of quiet sounds

                # Piecewise linear normalization
                levels = np.zeros(NUM_BANDS)
                for i, band in enumerate(bands):
                    if band < np.sqrt(MIN_ACTIVATION_THRESHOLD):
                        levels[i] = 0
                    elif band < np.sqrt(MAX_AMPLITUDE / 4):  # Quiet to moderate
                        levels[i] = (band - np.sqrt(MIN_ACTIVATION_THRESHOLD)) / (np.sqrt(MAX_AMPLITUDE / 4) - np.sqrt(MIN_ACTIVATION_THRESHOLD)) * 127
                    else:  # Moderate to loud
                        levels[i] = 127 + (band - np.sqrt(MAX_AMPLITUDE / 4)) / (np.sqrt(MAX_AMPLITUDE) - np.sqrt(MAX_AMPLITUDE / 4)) * 128
                    levels[i] = min(255, max(0, levels[i]))  # Scale to 0-255 for UDP

                # Apply smoothing
                levels = SMOOTHING_FACTOR * prev_levels + (1 - SMOOTHING_FACTOR) * levels
                prev_levels = levels

                # Convert to integers
                levels = levels.astype(int)

                # Debug amplitude
                # if np.max(bands) > 0:
                #     print(f"Max FFT: {np.max(fft):.0f}, Max band: {np.max(bands):.0f}, Max level: {np.max(levels)}")

                # Send to ESP32 via UDP
                packet = bytearray()
                packet.append(0x01)  # Magic number
                packet.append(NUM_BANDS)  # Number of bands
                for level in levels:
                    packet.append(level)
                sock.sendto(packet, (UDP_IP, UDP_PORT))
                print(f"Sent packet: {packet.hex()}")

                time.sleep(0.02)

            except IOError as e:
                print(f"Audio read error: {e}")
                continue

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        stream.stop_stream()
        stream.close()
        p.terminate()
        sock.close()
        print("Resources cleaned up.")

if __name__ == "__main__":
    main()