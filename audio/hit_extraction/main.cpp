#include <iostream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <cctype>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdint>

// Implementation flags must be defined before including in exactly one source file
#define DR_WAV_IMPLEMENTATION
#include "libs/dr_wav.h"
#define DR_MP3_IMPLEMENTATION
#include "libs/dr_mp3.h"
#define DR_FLAC_IMPLEMENTATION
#include "libs/dr_flac.h"

namespace fs = std::filesystem;

enum class FadeShape {
    Linear,
    Exponential,
    InverseExponential
};

struct Config {
    fs::path input_dir;
    fs::path output_dir;
    double fade_ms = 15.0;
    FadeShape shape = FadeShape::Exponential;
    double threshold_db = 10.0;
    double cooldown_ms = 150.0;
    int stride = 1; // Default to 1 (extract every item)
};

struct AudioBuffer {
    std::vector<float> data;
    unsigned int channels = 0;
    unsigned int sample_rate = 0;
    bool valid = false;
};

bool is_numeric(const std::string& str) {
    if (str.empty()) return false;
    size_t start = (str[0] == '-') ? 1 : 0;
    bool decimal = false;
    for (size_t i = start; i < str.size(); ++i) {
        if (str[i] == '.') {
            if (decimal) return false;
            decimal = true;
        } else if (!std::isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

// Unified decoding interface
AudioBuffer load_audio_file(const fs::path& file_path) {
    AudioBuffer audio;
    std::string ext = file_path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".wav") {
        drwav_uint64 total_pcm_frames;
        unsigned int channels;
        unsigned int sample_rate;
        float* sample_data = drwav_open_file_and_read_pcm_frames_f32(
            file_path.string().c_str(), &channels, &sample_rate, &total_pcm_frames, nullptr);
        
        if (sample_data) {
            audio.channels = channels;
            audio.sample_rate = sample_rate;
            audio.data.assign(sample_data, sample_data + (total_pcm_frames * channels));
            drwav_free(sample_data, nullptr);
            audio.valid = true;
        }
    } 
    else if (ext == ".mp3") {
        drmp3_uint64 total_pcm_frames;
        drmp3_config config;
        float* sample_data = drmp3_open_file_and_read_pcm_frames_f32(
            file_path.string().c_str(), &config, &total_pcm_frames, nullptr);
        
        if (sample_data) {
            audio.channels = config.channels;
            audio.sample_rate = config.sampleRate; // Capital R
            audio.data.assign(sample_data, sample_data + (total_pcm_frames * config.channels));
            drmp3_free(sample_data, nullptr);
            audio.valid = true;
        }
    } 
    else if (ext == ".flac") {
        drflac_uint64 total_pcm_frames;
        unsigned int channels;
        unsigned int sample_rate;
        float* sample_data = drflac_open_file_and_read_pcm_frames_f32(
            file_path.string().c_str(), &channels, &sample_rate, &total_pcm_frames, nullptr);
        
        if (sample_data) {
            audio.channels = channels;
            audio.sample_rate = sample_rate;
            audio.data.assign(sample_data, sample_data + (total_pcm_frames * channels));
            drflac_free(sample_data, nullptr);
            audio.valid = true;
        }
    }

    return audio;
}

void apply_fadeout(std::vector<float>& buffer, int channels, unsigned int sample_rate, double fade_ms, FadeShape shape) {
    size_t total_samples = buffer.size();
    size_t total_frames = total_samples / channels;
    size_t fade_frames = static_cast<size_t>((fade_ms / 1000.0) * sample_rate);
    
    if (fade_frames > total_frames) fade_frames = total_frames;
    if (fade_frames == 0) return;

    size_t fade_start_frame = total_frames - fade_frames;

    for (size_t f = 0; f < fade_frames; ++f) {
        size_t current_frame = fade_start_frame + f;
        // Progress from 0.0 (start of fade) to 1.0 (end of file)
        float progress = static_cast<float>(f) / static_cast<float>(fade_frames);
        float gain = 1.0f;

        if (shape == FadeShape::Linear) {
            gain = 1.0f - progress;
        } else if (shape == FadeShape::Exponential) {
            gain = std::pow(1.0f - progress, 3.0f);
        } else if (shape == FadeShape::InverseExponential) {
            // Holds high volume longer, drops sharply at the end
            gain = 1.0f - std::pow(progress, 3.0f);
        }

        for (int c = 0; c < channels; ++c) {
            buffer[current_frame * channels + c] *= gain;
        }
    }
}

void process_audio_file(const fs::path& file_path, const Config& config) {
    AudioBuffer audio = load_audio_file(file_path);
    if (!audio.valid || audio.data.empty()) return;

    size_t total_frames = audio.data.size() / audio.channels;

    const double attack_time = 0.008;
    const double release_time = 0.250;
    const double ga = std::exp(-1.0 / (attack_time * audio.sample_rate));
    const double gr = std::exp(-1.0 / (release_time * audio.sample_rate));
    
    const double threshold = std::pow(10.0, config.threshold_db / 20.0);
    const size_t min_slice_frames = static_cast<size_t>((config.cooldown_ms / 1000.0) * audio.sample_rate);
    const size_t max_slice_frames = static_cast<size_t>(1.000 * audio.sample_rate); 

    double envelope = 0.0;
    std::vector<size_t> trigger_indices;
    size_t last_trigger_frame = 0;

    for (size_t i = 0; i < total_frames; ++i) {
        float max_val = 0.0f;
        for (unsigned int c = 0; c < audio.channels; ++c) {
            max_val = std::max(max_val, std::abs(audio.data[i * audio.channels + c]));
        }

        double g = (max_val > envelope) ? ga : gr;
        envelope = g * envelope + (1.0 - g) * max_val;

        if (max_val > envelope * threshold && max_val > 0.015f) { 
            if (i - last_trigger_frame > min_slice_frames || trigger_indices.empty()) {
                trigger_indices.push_back(i);
                last_trigger_frame = i;
            }
        }
    }

    // Change the increment step from ++idx to += config.stride
    for (size_t idx = 0; idx < trigger_indices.size(); idx += config.stride) {
        size_t start_frame = trigger_indices[idx];
        
        const size_t max_lookback = static_cast<size_t>(0.005 * audio.sample_rate);
        for (size_t lookback = 0; lookback < max_lookback && (trigger_indices[idx] - lookback) > 0; ++lookback) {
            size_t check_frame = trigger_indices[idx] - lookback;
            size_t prev_frame = check_frame - 1;
            float current_val = audio.data[check_frame * audio.channels];
            float prev_val = audio.data[prev_frame * audio.channels];
            
            if ((current_val >= 0.0f && prev_val < 0.0f) || (current_val <= 0.0f && prev_val > 0.0f)) {
                start_frame = check_frame;
                break;
            }
        }

        // Calculate end frame based on the next chosen index, bounding to the next stride step
        size_t next_idx = idx + config.stride;
        size_t end_frame = (next_idx < trigger_indices.size()) ? trigger_indices[next_idx] : total_frames;
        if (end_frame - start_frame > max_slice_frames) end_frame = start_frame + max_slice_frames;

        size_t slice_frames = end_frame - start_frame;
        if (slice_frames < min_slice_frames) continue;

        size_t start_sample = start_frame * audio.channels;
        size_t slice_samples = slice_frames * audio.channels;

        std::vector<float> slice_buffer(audio.data.begin() + start_sample, audio.data.begin() + start_sample + slice_samples);
        apply_fadeout(slice_buffer, audio.channels, audio.sample_rate, config.fade_ms, config.shape);

        std::string out_filename = file_path.stem().string() + "_transient_" + std::to_string(idx) + ".wav";
        fs::path out_path = config.output_dir / out_filename;

        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT; 
        format.channels = audio.channels;
        format.sampleRate = audio.sample_rate;
        format.bitsPerSample = 32;                  

        drwav wav;
        if (drwav_init_file_write(&wav, out_path.string().c_str(), &format, nullptr)) {
            drwav_write_pcm_frames(&wav, slice_frames, slice_buffer.data());
            drwav_uninit(&wav);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_folder> <output_folder> [fade_ms] [stride] [linear|exponential] [sensitivity_db] [cooldown_ms]\n";
        return 1;
    }

    Config config;
    config.input_dir = argv[1];
    config.output_dir = argv[2];

    int arg_idx = 3;

    // Parse optional numeric parameters (fade_ms and stride)
    if (arg_idx < argc && is_numeric(argv[arg_idx])) {
        if (arg_idx + 1 < argc && is_numeric(argv[arg_idx + 1])) {
            config.fade_ms = std::stod(argv[arg_idx]);
            arg_idx++;
            int parsed_stride = std::stoi(argv[arg_idx]);
            if (parsed_stride > 0) config.stride = parsed_stride;
            arg_idx++;
        } else {
            double val = std::stod(argv[arg_idx]);
            if (std::string(argv[arg_idx]).find('.') != std::string::npos || val < 1.0) {
                config.fade_ms = val;
            } else {
                int parsed_stride = static_cast<int>(val);
                if (parsed_stride > 0) config.stride = parsed_stride;
            }
            arg_idx++;
        }
    }

    // Parse optional fade shape string
    if (arg_idx < argc) {
        std::string shape_arg = argv[arg_idx];
        std::transform(shape_arg.begin(), shape_arg.end(), shape_arg.begin(), ::tolower);
        if (shape_arg == "exponential" || shape_arg == "exp" || 
            shape_arg == "linear" || 
            shape_arg == "inverse_exponential" || shape_arg == "inv_exp") {
            
            if (shape_arg == "exponential" || shape_arg == "exp") {
                config.shape = FadeShape::Exponential;
            } else if (shape_arg == "inverse_exponential" || shape_arg == "inv_exp") {
                config.shape = FadeShape::InverseExponential;
            } else {
                config.shape = FadeShape::Linear;
            }
            arg_idx++;
        }
    }

    // Parse optional threshold and cooldown parameters
    if (arg_idx < argc && is_numeric(argv[arg_idx])) { 
        config.threshold_db = std::stod(argv[arg_idx]); 
        arg_idx++; 
    }
    if (arg_idx < argc && is_numeric(argv[arg_idx])) { 
        config.cooldown_ms = std::stod(argv[arg_idx]); 
        arg_idx++; 
    }

    // Verify directories
    if (!fs::exists(config.input_dir) || !fs::is_directory(config.input_dir)) {
        std::cerr << "Error: Input directory \"" << config.input_dir.string() << "\" does not exist.\n";
        return 1;
    }

    if (!fs::exists(config.output_dir)) {
        fs::create_directories(config.output_dir);
    }

    // Process matching audio formats recursively
    for (const auto& entry : fs::recursive_directory_iterator(config.input_dir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".wav" || ext == ".mp3" || ext == ".flac") {
                process_audio_file(entry.path(), config);
            }
        }
    }

    return 0;
}