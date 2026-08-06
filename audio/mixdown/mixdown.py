import os
import sys
import time
import wave
import numpy as np

def mix_tracks(input_paths, o_path):
    print(f"Mixing {len(input_paths)} layers...")

    opened_waves = [wave.open(fn, "rb") for fn in input_paths]
    try:
        # Reference parameters from the first track
        ref_params = opened_waves[0].getparams()
        ref_channels = ref_params.nchannels
        ref_rate = ref_params.framerate
        ref_width = ref_params.sampwidth

        processed_tracks = []
        min_frame_count = float('inf')

        for w in opened_waves:
            # Enforce matching format constraints
            if w.getnchannels() != ref_channels or w.getframerate() != ref_rate or w.getsampwidth() != ref_width:
                print(f"Skipping mismatching file: {w}")
                continue

            raw_frames = w.readframes(w.getnframes())
            data_1d = np.frombuffer(raw_frames, dtype="<i2").astype(np.float64)
            
            # Reshape into 2D layout: (frames, channels)
            data_2d = data_1d.reshape(-1, ref_channels)
            
            processed_tracks.append(data_2d)
            if data_2d.shape[0] < min_frame_count:
                min_frame_count = data_2d.shape[0]

        if not processed_tracks:
            raise ValueError("No valid tracks available to mix.")

        # Truncate all tracks evenly along the frame axis
        truncated_tracks = [t[:min_frame_count, :] for t in processed_tracks]

        # Stack along a new outer axis and average them down
        stacked = np.stack(truncated_tracks, axis=0)
        mixed = np.mean(stacked, axis=0)

        # Prevent 16-bit integer clipping/overflow anomalies
        mixed = np.clip(mixed, -32768, 32767)
        
        # Flatten back down to sequential interleaved format for raw export
        output_buffer = mixed.astype("<i2").tobytes()

        with wave.open(o_path, "w") as mix_wav:
            mix_wav.setparams(ref_params)
            mix_wav.setnframes(min_frame_count)
            mix_wav.writeframes(output_buffer)

    finally:
        for w in opened_waves:
            w.close()


def main():
    if len(sys.argv) < 2:
        print("Usage: script.py <input_dir> [output_path]")
        sys.exit(1)

    input_dir = sys.argv[1]
    input_paths = [
        os.path.join(input_dir, f)
        for f in os.listdir(input_dir)
        if f.lower().endswith(".wav")
    ]

    if not input_paths:
        print(f"No .wav files found in {input_dir}")
        sys.exit(1)

    if len(sys.argv) > 2:
        out_path = sys.argv[2]
    else:
        os.makedirs("temp", exist_ok=True)
        out_path = os.path.join("temp", f"{int(1000 * time.time())}.wav")

    mix_tracks(input_paths, out_path)


if __name__ == "__main__":
    main()