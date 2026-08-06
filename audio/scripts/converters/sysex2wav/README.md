# DX Sample Extractor (`sysex2wav`)

A C++ command-line tool built with JUCE that automatically batch-renders Yamaha DX synth SysEx (`.syx`) bank archives into sample sets (24-bit WAV) using a VST3 instrument host (e.g., [Dexed](https://asb2m10.github.io/dexed/)). Includes automatic SysEx patch name parsing, dynamic silence detection, and optional `.sfz` map generation.

---

## Features

* **Automated Batch Processing:** Recursively scans directories for `.syx` / `.mid` bank files and renders all 32 patches per bank.
* **Metadata Extraction:** Reads patch names directly from raw 32-voice DX7 SysEx headers to generate clean output directory structures.
* **Configurable Velocity & Gate Matrices:** Renders customizable MIDI note, velocity, and gate-length combinations per patch.
* **Offline Processing & Tail Trimming:** Utilizes JUCE offline block rendering and truncates trailing silence based on configurable dB thresholds.
* **SFZ Mapping Generator:** Generates `.sfz` instrument definition files with calculated key zones (`lokey`/`hikey`) and velocity ranges (`lovel`/`hivel`).

---

## Prerequisites

* **C++17 Compiler** (Clang, GCC, or MSVC)
* **CMake** (v3.15+)
* **JUCE 7 or 8** (linked via CMake `add_subdirectory` or `find_package`)
* **Dexed (or compatible DX7 VST3 plugin)**

---

## Building

```bash
# Generate build configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build executable
cmake --build build --config Release
```

The compiled binary should be located at:
* **macOS:** `./build/sysex2wav_artefacts/Release/sysex2wav`

---

## Usage

```bash
./sysex2wav <path_to_vst3> <input_bank_dir> <output_sample_dir> [--sfz]
```

### Examples

**Standard WAV Sample Extraction:**
```bash
./sysex2wav "/Library/Audio/Plug-Ins/VST3/Dexed.vst3" "./banks" "./samples_out"
```

**Extraction with `.sfz` File Generation:**
```bash
./sysex2wav "/Library/Audio/Plug-Ins/VST3/Dexed.vst3" "./banks" "./samples_out" --sfz
```

---

## Output Directory Structure

For a bank named `ROM1A.syx`, the output is structured as follows:

```text
samples_out/
└── ROM1A/
    ├── 01_BRASS___1/
    │   ├── 01_BRASS___1_N36_V40_G2.5s.wav
    │   ├── 01_BRASS___1_N36_V90_G2.5s.wav
    │   └── ...
    ├── 01_BRASS___1.sfz
    ├── 02_MARIMBA/
    └── ...
```

---

## Cross-Platform Considerations

The current setup contains macOS-specific linking configurations. Adapting for **Windows** or **Linux** requires minor adjustments to `CMakeLists.txt` and command invocation:

### 1. `CMakeLists.txt` Adjustments
Replace platform-specific framework linkages with conditional blocks:

```cmake
target_compile_definitions(sysex2wav PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0
)

# Platform-specific framework/library linking
if(APPLE)
    target_link_libraries(sysex2wav PRIVATE
        "-framework AudioUnit"
        "-framework CoreAudioKit"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Cocoa"
    )
elseif(WIN32)
    # JUCE handles standard Windows dependencies via CMake targets automatically
elseif(UNIX AND NOT APPLE)
    target_link_libraries(sysex2wav PRIVATE
        juce::juce_recommended_lto_flags
        # Ensure ALSA / X11 dev libraries are installed via package manager
    )
endif()
```

### 2. Plugin Path Format
* **macOS:** Points to the `.vst3` bundle directory (e.g., `/Library/Audio/Plug-Ins/VST3/Dexed.vst3`).
* **Windows:** Points to the `.vst3` directory or underlying binary DLL (e.g., `C:\Program Files\Common Files\VST3\Dexed.vst3`).
* **Linux:** Points to the `.vst3` folder or `.so` binary (e.g., `~/.vst3/Dexed.vst3`).