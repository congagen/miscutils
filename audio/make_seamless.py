import os
import sys
import pydub
from pydub import AudioSegment


def fade_mix(in_slice, out_slice, fade_dur):
    inns = in_slice.fade_in(int(fade_dur))
    outs = out_slice.fade_out(int(fade_dur))

    return inns.overlay(outs, position=0)  # position=0.1 * len(out_slice)


def main():
    try:
        in_path  = sys.argv[1]
        fade_dur = float(sys.argv[2])
        out_path = sys.argv[3]

        sound = AudioSegment.from_wav(in_path)
        duration_mils = len(sound)  # sound.duration_seconds  # == (len(sound) / 1000.0)

        inout_fade_dur = int(duration_mils * fade_dur)
        middle = int(duration_mils - inout_fade_dur)

        intro_slice = sound[0: inout_fade_dur]
        middl_slice = sound[inout_fade_dur:middle]
        outro_slice = sound[middle: duration_mils]

        intro_fade = fade_mix(intro_slice, outro_slice, inout_fade_dur)

        concat = intro_fade + middl_slice
        concat.export(out_path, format="wav")
    except:
        print("Missing args? (1: Input path, 2: Fade Duration, 3: Output Path)")


if __name__ == "__main__":
    main()
