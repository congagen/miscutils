Audio Transient Extractor
A lightweight, highly portable command-line tool written in C++17 for recursive audio transient extraction. The application crawls directories, detects sudden energy jumps using an envelope follower, applies customizable fade-out curves, and extracts individual slices to disk as standard WAV files.

Features
Recursive Processing: Scans input directories recursively to batch-process audio collections.

Adaptive Detection: Dynamically reads channel layouts and sample rates to calculate accurate time constants for the envelope follower.

Configurable Slices: Supports linear or exponential fade-out shapes to avoid clicks on cut points.

Zero System Dependencies: Uses vendor-packaged single-header decoders inside the repository. No external package managers or system libraries are required.

Supported Input Formats: WAV (PCM), MP3, FLAC

Note: All extracted transient slices are written out as standard 16-bit PCM WAV files.

Project Structure
[Structure Start]
├── libs/
│   ├── dr_flac.h
│   ├── dr_mp3.h
│   └── dr_wav.h
├── main.cpp
└── CMakeLists.txt
[Structure End]

Prerequisites
A C++17 compliant compiler (e.g., g++ 8+, clang 7+, or MSVC 2017+).

[Optional] CMake 3.12+ for cross-platform build generation.

Building
Method 1: Using CMake (Recommended for Cross-Platform)
Verify that cmake is installed and accessible in your terminal by running:

cmake --version

If the command is not found on macOS, link your existing installation by running sudo ln -s /Applications/CMake.app/Contents/bin/cmake /usr/local/bin/cmake or install it via a package manager.

Once verified, build the project from the root directory:

mkdir build && cd build
cmake ..
cmake --build .

Method 2: Direct Compiler Call
If you prefer a quick compile without generating build files, pass the include path for the embedded headers directly:

g++ -std=c++17 main.cpp -I./libs -o transient_extractor

Here is the complete, self-contained documentation you can swap directly into your README.md:

Usage
Run the compiled executable from your terminal by passing the required input and output paths, along with optional configuration parameters:

[Command Start]
./transient_extractor <input_folder> <output_folder> [fade_ms] [stride] [linear|exponential|inverse_exponential] [sensitivity_db] [cooldown_ms]
[Command End]

Parameter Details & Recommended Values
<input_folder> (Required): Path to the target directory containing the audio source files. Scans recursively through all subdirectories for .wav, .mp3, and .flac files.

<output_folder> (Required): Path where the extracted transient slices will be saved. The directory will be generated automatically if it does not exist.

[fade_ms] (Default: 15.0): The length of the fade-out window (in milliseconds) applied to the very end of each extracted slice to eliminate audible zero-crossing clicks.

[stride] (Default: 1): The indexing step size for slice extraction.

A value of 1 extracts every single transient found.

A value of 5 or 10 skips consecutive triggers inside a single file, capturing only every 5th or 10th transient.

[linear|exponential|inverse_exponential] (Default: exponential): The volume envelope shape applied during the fade-out window:

exponential (or exp): Drops off quickly right after the peak. Ideal for sharp, clean acoustic percussion decays.

inverse_exponential (or inv_exp): Holds the volume near maximum throughout the slice body, dropping off sharply at the absolute tail. Ideal for electronic drums and sustained synth stabs.

linear: Applies a straight uniform decay ramp.

[sensitivity_db] (Default: 10.0): The threshold factor in decibels above the localized envelope floor required to register a transient spike.

Aacoustic drums / isolated hits: Try 8.0 to 12.0.

Complex loops / dense stems: Try 14.0 to 18.0 to avoid over-triggering on faint background noise or ghost notes.

[cooldown_ms] (Default: 150.0): The minimum mandatory duration required between consecutive triggers to prevent a single complex attack transient from double-triggering.

Fast material (e.g., hi-hats, breakbeats): Lower this to 50.0 or 80.0.

Isolated notes (e.g., guitar chord stabs): Raise this to 250.0 or 400.0.

Examples
1. Default Behavior
Extract every single transient found inside the raw_samples directory using a standard exponential curve:
[Command Start]
./transient_extractor raw_samples extracted_slices
[Command End]

2. High Density Downsampling
Process the input folder, but only capture every 10th detected transient (using a 15ms fade-out) to gather a sparse, diverse sample collection without flooding the disk:
[Command Start]
./transient_extractor raw_samples extracted_slices 15 10
[Command End]

3. Sustained Hit Extraction
Apply an inverse exponential curve with a long 500ms tail to ensure the body of sustained electronic hits or loops isn't choked prematurely:
[Command Start]
./transient_extractor raw_samples extracted_slices 500 1 inv_exp
[Command End]