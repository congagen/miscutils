import os
import sys
import pydub
from pydub import AudioSegment


def fade_mix(in_slice, out_slice, fade_dur):
    inns = in_slice.fade_in(int(fade_dur))
    outs = out_slice.fade_out(int(fade_dur))

    return inns.overlay(outs, position=0)  # position=0.1 * len(out_slice)


def mix_seamless(in_path, fade_dur, out_path="", filename=""):
    sound = AudioSegment.from_wav(in_path)
    duration_mils = len(sound) #sound.duration_seconds  # == (len(sound) / 1000.0)

    inout_fade_dur = int(duration_mils * fade_dur)
    middle = int(duration_mils - inout_fade_dur)

    intro_slice = sound[0: inout_fade_dur]
    middl_slice = sound[inout_fade_dur:middle]
    outro_s = sound[middle: duration_mils]

    intro_slice.export(out_path + filename + "__in.wav", format="wav")
    middl_slice.export(out_path + filename + "_mid.wav", format="wav")
    outro_s.export(out_path + filename + "_out.wav", format="wav")

    intro_fade = fade_mix(intro_slice, outro_s, inout_fade_dur)

    concat = intro_fade + middl_slice
    concat.export(out_path + filename + "_mixed.wav", format="wav")


def main(fade_dur):
    try:
        mix_seamless(sys.argv[1], float(sys.argv[2]), sys.argv[3], sys.argv[4])
    except:
        print("Error: Missing args (1: Infile path, 2: Fade Duration, 3: Out Path, 4: Out filename)")


if __name__ == "__main__":
    main(0.2)
