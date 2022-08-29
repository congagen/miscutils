import os
import sys
import pydub
from pydub import AudioSegment


def main(in_path, duration_ms, out_path, fade_out=0.025):
    raw_audio = AudioSegment.from_wav(in_path)
    infile_duration_ms = len(raw_audio)
    loop_count = 0
    mixdown = raw_audio

    if infile_duration_ms < duration_ms:
        loop_count = int(duration_ms / infile_duration_ms)

    for i in range(loop_count):
        print("+" + str(i) + "...")
        mixdown += raw_audio

    final_slice = mixdown[0: duration_ms]
    final_slice.fade_out(int(infile_duration_ms * fade_out))

    final_slice.export(out_path, format="wav")


if __name__ == "__main__":
    try:
        main(sys.argv[1], int(sys.argv[2]), sys.argv[3])
    except:
        print("Missing args? (1: Infile path, 2: Duration (mils), 3: Out Path")
