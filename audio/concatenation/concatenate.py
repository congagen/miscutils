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
    if not data_list:
        raise ValueError("Input file list is empty.")

    compiled_audio = bytearray()
    compo_sample_dur = 0.0
    prev_s_name = ""
    
    # Open initial file to read target properties
    with wave.open(data_list[0], 'rb') as first_w:
        params = first_w.getparams()
        sample_rate = first_w.getframerate()
        sample_width = first_w.getsampwidth()
        n_channels = first_w.getnchannels()

    bytes_per_frame = sample_width * n_channels

    while compo_sample_dur < duration:
        for infile in data_list:
            if compo_sample_dur >= duration:
                break

            if mode == 0:
                selected = infile
            else:
                if destall and len(data_list) > 1:
                    s_list_copy = [i for i in data_list if i != prev_s_name]
                    selected = random.choice(s_list_copy)
                else:
                    selected = random.choice(data_list)
                prev_s_name = selected

            with wave.open(selected, 'rb') as w:
                # Ensure input formats match target format
                if w.getframerate() != sample_rate or w.getsampwidth() != sample_width or w.getnchannels() != n_channels:
                    continue
                
                # Append audio content
                frames = w.readframes(w.getnframes())
                compiled_audio.extend(frames)

            # Calculate offset padding
            item_offset = min_offset
            if max_offset > min_offset:
                item_offset = random.uniform(min_offset, max_offset)

            # Generate and append silence matching formatting properties
            padding_frames = int(item_offset * sample_rate)
            silence_bytes = b'\x00' * (padding_frames * bytes_per_frame)
            compiled_audio.extend(silence_bytes)

            # Accurate duration tracking
            total_frames = len(compiled_audio) // bytes_per_frame
            compo_sample_dur = total_frames / sample_rate

    # Truncate content if it exceeds the maximum duration limit
    max_total_bytes = int(duration * sample_rate) * bytes_per_frame
    if len(compiled_audio) > max_total_bytes:
        compiled_audio = compiled_audio[:max_total_bytes]

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    
    with wave.open(out_path, 'wb') as output:
        output.setparams(params)
        output.setnframes(len(compiled_audio) // bytes_per_frame)
        output.writeframes(bytes(compiled_audio))

    return os.path.abspath(out_path)


def get_sysarg(arg_idx, default):
    if len(sys.argv) > arg_idx:
        return sys.argv[arg_idx]
    return default


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: script.py <input_dir> [duration] [offset] [mode]")
        sys.exit(1)

    input_dir = sys.argv[1]
    duration = float(get_sysarg(2, 60.0))
    offset = float(get_sysarg(3, 2.0))
    mode = int(get_sysarg(4, 1))

    if not os.path.isdir(input_dir):
        print(f"Directory not found: {input_dir}")
        sys.exit(1)

    data_list = [os.path.join(input_dir, f) for f in os.listdir(input_dir) if f.lower().endswith(".wav")]
    
    out_path = os.path.join("temp", f"{int(1000 * time.time())}.wav")
    res_path = concatenate(data_list, duration, offset, mode, out_path)
    print(f"Output saved to: {res_path}")

if __name__ == "__main__":
    input_dir = sys.argv[1]
    duration = 60 if len(sys.argv) < 3 else get_sysarg(2)
    offset = 2 if len(sys.argv) < 4 else get_sysarg(3)
    mode = 1 if len(sys.argv) < 5 else get_sysarg(4)

    input_paths = list(input_dir+"/"+f for f in os.listdir(input_dir) if f.endswith("wav"))
    data_list = input_paths

    out_path = "temp/" + str(int(1000 * time.time())) + ".wav"
    res_path = concatenate(data_list, int(duration), float(offset), int(mode), out_path)
