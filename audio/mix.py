import wave
import time
import sys
import os
import numpy as np


def write_audio(file_path, audio_data, frame_count=0, num_chan=2, s_rate=44100):
    n_chan = max(min(num_chan, 1), 2)
    frame_count = len(audio_data) if frame_count == 0 else frame_count

    f = wave.open(file_path, 'w')
    f.setparams((n_chan, 2, s_rate, frame_count, "NONE", "Uncompressed"))
    f.writeframes(audio_data.tobytes())
    f.close()


def mix_tracks(input_paths, o_path, max_amplitude=30000):
    print("Mixing layers...")

    w = wave.open(input_paths[0], 'rb')
    waves = [wave.open(fn) for fn in input_paths]
    frames = [w.readframes(w.getnframes()) for w in waves]

    layer_count = len(frames)

    samples = [np.frombuffer(f, dtype='<i2') for f in frames]
    samples = [samp.astype(np.float64) for samp in samples]
    n = min(map(len, samples))

    mix = samples[0][:n]
    mix = np.true_divide(mix, layer_count)

    for s in range(1, len(samples)):
        m = samples[s][:n]
        m = np.true_divide(m, layer_count)
        mix += m

    mix_wav = wave.open(o_path, 'w')
    mix_wav.setparams(w.getparams())
    mix_wav.writeframes(mix.astype('<i2').tobytes())
    mix_wav.close()


if __name__ == "__main__":
    frame_limit = 0
    input_dir = sys.argv[1]
    frame_tracks = []

    max_frames = 0
    num_chns = 1
    change = 0

    input_paths = list("data/"+f for f in os.listdir(input_dir) if f.endswith("wav"))

    if len(sys.argv) > 2:
        out_path = sys.argv[2]
    else:
        out_path = "temp/" + str(int(1000*time.time())) + ".wav"

    mix_tracks(input_paths, out_path)
