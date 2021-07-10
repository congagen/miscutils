import wave
import time
import sys
import os
import random
import struct


def write_audio(file_path, audio_data, frame_count=0, num_chan=2, s_rate=44100):
    n_chan = max(min(num_chan, 1), 2)
    frame_count = len(audio_data) if frame_count == 0 else frame_count

    f = wave.open(file_path, 'w')
    f.setparams((n_chan, 2, s_rate, frame_count, "NONE", "Uncompressed"))
    f.writeframes(audio_data.tobytes())
    f.close()


def concatenate(data_list, duration, min_offset, mode, out_path, max_offset=0.0, destall=True):
    data = []
    #silence_m = wave.open("silence_mono.wav", 'rb')
    #silence_m_frames = silence_m.readframes(silence_m.getnframes())

    compo_sample_dur = 0
    prev_s_name = ""
    item_offset = min_offset

    while compo_sample_dur < duration:
        for infile in data_list:
            if mode == 0:
                w = wave.open(infile, 'rb')
            else:
                if destall:
                    s_list_copy = [i for i in data_list if i != prev_s_name]
                    selected = random.choice(s_list_copy)
                    w = wave.open(selected, 'rb')
                    prev_s_name = selected
                else:
                    selected = random.choice(data_list)
                    w = wave.open(selected, 'rb')

            if max_offset > 0.1:
                item_offset = min_offset + float(random.uniform(0, max_offset))
                print(item_offset)

            sample_rate = w.getframerate()
            frame_count = w.getnframes()

            data.append([w.getparams(), w.readframes(w.getnframes())])

            for f in range(int(item_offset * sample_rate)):
                data.append([w.getparams(), struct.pack('B', 0)])

            compo_sample_dur = (len(data) / sample_rate)
            print("Progress: " + str(duration) + " / " + str(compo_sample_dur))
            w.close()

            if compo_sample_dur > duration:
                break

    output = wave.open(out_path, 'wb')
    output.setparams(data[0][0])

    for i in range(len(data)):
        output.writeframes(data[i][1])

    output.close()
    return os.path.abspath(out_path)


def get_sysarg(arg_idx):
    if len(sys.argv) > arg_idx:
        return sys.argv[arg_idx]
    else:
        return 1


if __name__ == "__main__":
    input_dir = sys.argv[1]
    duration = 60 if len(sys.argv) < 3 else get_sysarg(2)
    offset = 2 if len(sys.argv) < 4 else get_sysarg(3)
    mode = 1 if len(sys.argv) < 5 else get_sysarg(4)

    input_paths = list(input_dir+"/"+f for f in os.listdir(input_dir) if f.endswith("wav"))
    data_list = input_paths

    out_path = "temp/" + str(int(1000 * time.time())) + ".wav"
    res_path = concatenate(data_list, int(duration), float(offset), int(mode), out_path)
