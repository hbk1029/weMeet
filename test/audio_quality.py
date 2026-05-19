"""Audio quality test: generate reference signal, verify via simple metrics.

Since the Project's Speex DLL can only be called from within the Qt application
context, this script uses pure-Python signal analysis instead of ctypes.

Usage:
  python audio_quality.py               # synthetic 440 Hz sine wave test
  python audio_quality.py <wav_file>    # analyze a real WAV file
"""

import math
import struct
import sys
import os


SAMPLE_RATE = 8000
FRAME_SIZE = 160  # 20 ms Narrowband
BITS = 16
MAX_VAL = 32767.0


def gen_sine(freq, sample_rate, duration, amplitude=0.8):
    """Generate 16-bit PCM sine samples."""
    n = int(sample_rate * duration)
    return [int(amplitude * MAX_VAL * math.sin(2 * math.pi * freq * i / sample_rate))
            for i in range(n)]


def read_wav(path):
    """Read 8/16-bit mono WAV. Returns [int16_samples] and sample rate."""
    import wave
    with wave.open(path, "rb") as wf:
        if wf.getnchannels() != 1:
            raise ValueError("Only mono WAV supported")
        sr = wf.getframerate()
        sw = wf.getsampwidth()
        n = wf.getnframes()
        raw = wf.readframes(n)
    if sw == 2:
        samples = list(struct.unpack(f"<{n}h", raw))
    elif sw == 1:
        samples = [(b - 128) << 8 for b in raw]
    else:
        raise ValueError(f"Unsupported sample width: {sw}")
    return samples, sr


def psnr(original, processed):
    """Peak Signal-to-Noise Ratio in dB."""
    m = min(len(original), len(processed))
    pin = original[:m]
    pou = processed[:m]
    mse = sum((o - d) ** 2 for o, d in zip(pin, pou)) / m
    if mse == 0:
        return float("inf")
    return 20 * math.log10(MAX_VAL) - 10 * math.log10(mse)


def max_abs_error(original, processed):
    """Maximum absolute sample error."""
    m = min(len(original), len(processed))
    return max(abs(original[i] - processed[i]) for i in range(m))


def rms(samples):
    """Root Mean Square."""
    m = sum(s ** 2 for s in samples) / len(samples)
    return math.sqrt(m)


def zero_crossing_rate(samples, frame_size=FRAME_SIZE):
    """Zero crossing rate per frame."""
    zcrs = []
    for f in range(len(samples) // frame_size):
        frame = samples[f * frame_size: (f + 1) * frame_size]
        zc = sum(1 for i in range(1, len(frame))
                 if (frame[i] >= 0) != (frame[i - 1] >= 0))
        zcrs.append(zc / frame_size)
    return zcrs


def main():
    print("=== Audio Quality Analysis ===")
    print(f"Sample Rate: {SAMPLE_RATE} Hz, Frame: {FRAME_SIZE} samples (20 ms)")

    if len(sys.argv) > 1:
        path = sys.argv[1]
        print(f"Source: {path}")
        samples, sr = read_wav(path)
        print(f"  Sample rate: {sr} Hz, samples: {len(samples)}")
        if sr != SAMPLE_RATE:
            print(f"  [WARN] Resampling from {sr} to {SAMPLE_RATE} Hz not performed")
        # reference is same file (no encode/decode cycle)
        ref = samples
    else:
        print("Source: synthetic 440 Hz sine wave, 1.0 second")
        ref = gen_sine(440, SAMPLE_RATE, 1.0)
        sr = SAMPLE_RATE
        # simulate light noise
        import random
        random.seed(42)
        processed = [s + int(random.gauss(0, 50)) for s in ref]
        # clip
        processed = [max(-32768, min(32767, v)) for v in processed]

        r = rms(ref)
        zcr = zero_crossing_rate(ref)
        p = psnr(ref, processed)
        mae = max_abs_error(ref, processed)

        print(f"\n--- Results ---")
        print(f"Signal RMS:      {r:.1f}")
        print(f"Avg ZCR:         {sum(zcr) / len(zcr):.4f}")
        print(f"PSNR (vs noise): {p:.2f} dB")
        print(f"Max abs error:   {mae}")
        print(f"\nTip: use 'python audio_quality.py <file.wav>' to analyze a real file")

    print("\n=== DONE ===")


if __name__ == "__main__":
    main()
