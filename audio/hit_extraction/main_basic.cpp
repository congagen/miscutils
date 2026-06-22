#include <iostream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdint>

namespace fs = std::filesystem;

enum class FadeShape {
    Linear,
    Exponential
};

struct Config {
    fs::path input_dir;
    fs::path output_dir;
    double fade_ms = 15.0; // Optimized default fade
    FadeShape shape = FadeShape::Exponential;
    double threshold_db = 10.0; // Happy-medium sensitivity for varied music
    double cooldown_ms = 150.0; // Clean minimum interval for general arrangements
};

#pragma pack(push, 1)
struct WavHeader {
    char chunk_id[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunk_size = 0;
    char format[4] = {'W', 'A', 'V', 'E'};
    char subchunk1_id[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; 
    uint16_t num_channels = 1;
    uint32_t sample_rate = 44100;
    uint32_t byte_rate = 0;
    uint16_t block_align = 0;
    uint16_t bits_per_sample = 16;
    char subchunk2_id[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2_size = 0;
};
#pragma pack(pop)

void apply_fadeout(std::vector<float>& buffer, int channels, int sample_rate, double fade_ms, FadeShape shape) {
    size_t total_samples = buffer.size();
    size_t total_frames = total_samples / channels;
    size_t fade_frames = static_cast<size_t>((fade_ms / 1000.0) * sample_rate);
    
    if (fade_frames > total_frames) fade_frames = total_frames;
    size_t fade_start_frame = total_frames - fade_frames;

    for (size_t i = 0; i < fade_frames; ++i) {
        size_t current_frame = fade_start_frame + i;
        double progress = static_cast<double>(i) / fade_frames;
        float gain = 1.0f;

        if (shape == FadeShape::Linear) {
            gain = static_cast<float>(1.0 - progress);
        } else if (shape == FadeShape::Exponential) {
            gain = static_cast<float>(std::exp(-5.0 * progress));
        }

        for (int c = 0; c < channels; ++c) {
            buffer[current_frame * channels + c] *= gain;
        }
    }
}

void process_wav_file(const fs::path& file_path, const Config& config) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) return;

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    if (std::string(header.chunk_id, 4) != "RIFF" || std::string(header.format, 4) != "WAVE") return;
    if (header.audio_format != 1 && header.audio_format != 3) return;

    std::vector<char> raw_data(header.subchunk2_size);
    file.read(raw_data.data(), header.subchunk2_size);
    file.close();

    size_t bytes_per_sample = header.bits_per_sample / 8;
    size_t total_samples = header.subchunk2_size / bytes_per_sample;
    size_t total_frames = total_samples / header.num_channels;
    if (total_frames == 0) return;

    std::vector<float> input_buffer(total_samples);
    for (size_t i = 0; i < total_samples; ++i) {
        size_t offset = i * bytes_per_sample;
        if (header.audio_format == 1) { 
            if (header.bits_per_sample == 16) {
                int16_t sample = *reinterpret_cast<int16_t*>(&raw_data[offset]);
                input_buffer[i] = sample / 32768.0f;
            } else if (header.bits_per_sample == 24) {
                int32_t sample = (raw_data[offset] & 0xFF) | 
                                 ((raw_data[offset + 1] & 0xFF) << 8) | 
                                 (raw_data[offset + 2] << 16);
                if (sample & 0x00800000) sample |= 0xFF000000; 
                input_buffer[i] = sample / 8388608.0f;
            } else if (header.bits_per_sample == 32) {
                int32_t sample = *reinterpret_cast<int32_t*>(&raw_data[offset]);
                input_buffer[i] = sample / 2147483648.0f;
            }
        } else if (header.audio_format == 3 && header.bits_per_sample == 32) { 
            input_buffer[i] = *reinterpret_cast<float*>(&raw_data[offset]);
        }
    }

    // Adaptively tuned coefficients for complex musical structures
    const double attack_time = 0.008;  // Snappy 8ms attack tracking
    const double release_time = 0.250; // Slower 250ms release prevents mid-note re-triggering
    const double ga = std::exp(-1.0 / (attack_time * header.sample_rate));
    const double gr = std::exp(-1.0 / (release_time * header.sample_rate));
    
    const double threshold = std::pow(10.0, config.threshold_db / 20.0);
    const size_t min_slice_frames = static_cast<size_t>((config.cooldown_ms / 1000.0) * header.sample_rate);
    const size_t max_slice_frames = static_cast<size_t>(1.000 * header.sample_rate); 

    double envelope = 0.0;
    std::vector<size_t> trigger_indices;
    size_t last_trigger_frame = 0;

    for (size_t i = 0; i < total_frames; ++i) {
        float max_val = 0.0f;
        for (int c = 0; c < header.num_channels; ++c) {
            max_val = std::max(max_val, std::abs(input_buffer[i * header.num_channels + c]));
        }

        double g = (max_val > envelope) ? ga : gr;
        envelope = g * envelope + (1.0 - g) * max_val;

        // Dynamic spike comparison + hard gate floor at -36dB to block noise floors
        if (max_val > envelope * threshold && max_val > 0.015f) { 
            if (i - last_trigger_frame > min_slice_frames || trigger_indices.empty()) {
                trigger_indices.push_back(i);
                last_trigger_frame = i;
            }
        }
    }

    for (size_t idx = 0; idx < trigger_indices.size(); ++idx) {
        size_t start_frame = trigger_indices[idx];
        
        // Phase-preserving zero crossing check (5ms window)
        const size_t max_lookback = static_cast<size_t>(0.005 * header.sample_rate);
        for (size_t lookback = 0; lookback < max_lookback && (trigger_indices[idx] - lookback) > 0; ++lookback) {
            size_t check_frame = trigger_indices[idx] - lookback;
            size_t prev_frame = check_frame - 1;
            float current_val = input_buffer[check_frame * header.num_channels];
            float prev_val = input_buffer[prev_frame * header.num_channels];
            
            if ((current_val >= 0.0f && prev_val < 0.0f) || (current_val <= 0.0f && prev_val > 0.0f)) {
                start_frame = check_frame;
                break;
            }
        }

        size_t end_frame = (idx + 1 < trigger_indices.size()) ? trigger_indices[idx + 1] : total_frames;
        if (end_frame - start_frame > max_slice_frames) end_frame = start_frame + max_slice_frames;

        size_t slice_frames = end_frame - start_frame;
        if (slice_frames < min_slice_frames) continue;

        size_t start_sample = start_frame * header.num_channels;
        size_t slice_samples = slice_frames * header.num_channels;

        std::vector<float> slice_buffer(input_buffer.begin() + start_sample, input_buffer.begin() + start_sample + slice_samples);
        apply_fadeout(slice_buffer, header.num_channels, header.sample_rate, config.fade_ms, config.shape);

        std::vector<char> out_raw_data(slice_samples * bytes_per_sample);
        for (size_t i = 0; i < slice_samples; ++i) {
            size_t offset = i * bytes_per_sample;
            if (header.audio_format == 1) {
                if (header.bits_per_sample == 16) {
                    int16_t sample = static_cast<int16_t>(std::clamp(slice_buffer[i] * 32767.0f, -32768.0f, 32767.0f));
                    *reinterpret_cast<int16_t*>(&out_raw_data[offset]) = sample;
                } else if (header.bits_per_sample == 24) {
                    int32_t sample = static_cast<int32_t>(std::clamp(slice_buffer[i] * 8388607.0f, -8388608.0f, 8388607.0f));
                    out_raw_data[offset] = sample & 0xFF;
                    out_raw_data[offset + 1] = (sample >> 8) & 0xFF;
                    out_raw_data[offset + 2] = (sample >> 16) & 0xFF;
                } else if (header.bits_per_sample == 32) {
                    int32_t sample = static_cast<int32_t>(std::clamp(slice_buffer[i] * 2147483647.0f, -2147483648.0f, 2147483647.0f));
                    *reinterpret_cast<int32_t*>(&out_raw_data[offset]) = sample;
                }
            } else if (header.audio_format == 3) {
                *reinterpret_cast<float*>(&out_raw_data[offset]) = slice_buffer[i];
            }
        }

        std::string out_filename = file_path.stem().string() + "_transient_" + std::to_string(idx) + ".wav";
        fs::path out_path = config.output_dir / out_filename;

        WavHeader out_header = header;
        out_header.subchunk2_size = static_cast<uint32_t>(out_raw_data.size());
        out_header.chunk_size = out_header.subchunk2_size + sizeof(WavHeader) - 8;

        std::ofstream out_file(out_path, std::ios::binary);
        if (out_file) {
            out_file.write(reinterpret_cast<const char*>(&out_header), sizeof(WavHeader));
            out_file.write(out_raw_data.data(), out_raw_data.size());
            out_file.close();
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <input_folder> <output_folder> [fade_ms] [linear|exponential] [sensitivity_db] [cooldown_ms]\n";
        return 1;
    }

    Config config;
    config.input_dir = argv[1];
    config.output_dir = argv[2];
    
    // Fall back smoothly to safe defaults if manual values aren't parsed
    if (argc > 3) config.fade_ms = std::stod(argv[3]);
    if (argc > 4) {
        std::string shape_arg = argv[4];
        std::transform(shape_arg.begin(), shape_arg.end(), shape_arg.begin(), ::tolower);
        if (shape_arg == "exponential" || shape_arg == "exp") config.shape = FadeShape::Exponential;
        else config.shape = FadeShape::Linear;
    }
    if (argc > 5) config.threshold_db = std::stod(argv[5]);
    if (argc > 6) config.cooldown_ms = std::stod(argv[6]);

    if (!fs::exists(config.output_dir)) fs::create_directories(config.output_dir);

    for (const auto& entry : fs::recursive_directory_iterator(config.input_dir)) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".wav") {
            process_wav_file(entry.path(), config);
        }
    }
    return 0;
}