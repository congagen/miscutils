import os
import sys
import pydub
from pydub import AudioSegment


def main(in_path, duration_ms, out_path, fade_out_ratio=0.025):
    if not os.path.exists(in_path):
        print(f"Error: Input file not found at {in_path}")
        sys.exit(1)

    raw_audio = AudioSegment.from_wav(in_path)
    infile_duration_ms = len(raw_audio)

    if infile_duration_ms >= duration_ms:
        # File is already longer than target duration
        final_slice = raw_audio[:duration_ms]
    else:
        # Calculate precise multiplier needed to exceed or match target
        loops_needed = math.ceil(duration_ms / infile_duration_ms)
        mixdown = raw_audio * loops_needed
        final_slice = mixdown[:duration_ms]

    # Calculate fade length safely relative to final duration
    fade_ms = int(duration_ms * fade_out_ratio)
    if fade_ms > 0:
        final_slice = final_slice.fade_out(duration_=fade_ms)

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    final_slice.export(out_path, format="wav")
    print(f"Exported looped audio to: {out_path}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: script.py <input_path> <duration_ms> <output_path>")
        sys.exit(1)

    try:
        target_duration = int(sys.argv[2])
    except ValueError:
        print("Error: Duration must be an integer (milliseconds).")
        sys.exit(1)

    main(sys.argv[1], target_duration, sys.argv[3])