import os
import sys
import pydub
from pydub import AudioSegment


def main():
    if len(sys.argv) < 4:
        print("Usage: script.py <input_path> <fade_duration_fraction> <output_path>")
        sys.exit(1)

    try:
        in_path = sys.argv[1]
        fade_fraction = float(sys.argv[2])
        out_path = sys.argv[3]

        if not os.path.exists(in_path):
            print(f"Error: Input file not found at {in_path}", file=sys.stderr)
            sys.exit(1)

        sound = AudioSegment.from_wav(in_path)
        duration_mils = len(sound)

        # Calculate fade duration from fraction
        crossfade_time = int(duration_mils * fade_fraction)
        if crossfade_time * 2 >= duration_mils:
            print("Error: Fade duration fraction is too large for file length.", file=sys.stderr)
            sys.exit(1)

        # Isolate the tail and head sections to blend
        intro_slice = sound[:crossfade_time]
        middl_slice = sound[crossfade_time:-crossfade_time]
        outro_slice = sound[-crossfade_time:]

        # Seamless Loop Assembly:
        # Start with the middle body.
        # Append the tail (outro), crossfading it directly into the head (intro).
        # When played on repeat, the end of this new file matches the start of middl_slice.
        looped_sound = middl_slice.append(outro_slice.append(intro_slice, crossfade=crossfade_time), crossfade=0)

        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        looped_sound.export(out_path, format="wav")
        print(f"Seamless loop exported to: {out_path}")

    except Exception as e:
        print(f"An error occurred: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()