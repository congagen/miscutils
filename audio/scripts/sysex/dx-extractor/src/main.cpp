#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <iostream>
#include <vector>
#include <string>

struct SamplingConfig
{
    double sampleRate = 44100.0;
    int bitDepth = 24;
    std::vector<int> midiNotes = { 36, 48, 60, 72, 84 }; // C1, C2, C3, C4, C5
    std::vector<int> velocities = { 40, 90, 127 };
    std::vector<float> gateLengthsSec = { 0.3f, 2.5f };
    float maxReleaseSec = 6.0f;
    float silenceThresholdDb = -70.0f;
    bool generateSfz = false;
};

juce::String getPatchName(const uint8_t* voiceData)
{
    char nameBuf[11];
    for (int i = 0; i < 10; ++i)
    {
        char c = static_cast<char>(voiceData[118 + i]);
        nameBuf[i] = (c >= 32 && c <= 126) ? c : '_';
    }
    nameBuf[10] = '\0';
    juce::String patchName = juce::String(nameBuf).trim();
    return patchName.isEmpty() ? "UNNAMED" : juce::File::createLegalFileName(patchName);
}

juce::MemoryBlock extractVoiceSysEx(const juce::MemoryBlock& bankData, int patchIndex)
{
    juce::MemoryBlock sysEx;
    size_t dataOffset = 6 + (patchIndex * 128);
    if (bankData.getSize() < dataOffset + 128)
        return sysEx;

    const uint8_t header[] = { 0xF0, 0x43, 0x00, 0x00, 0x01, 0x1B };
    sysEx.append(header, sizeof(header));
    sysEx.append(static_cast<const uint8_t*>(bankData.getData()) + dataOffset, 128);
    
    const uint8_t footer = 0xF7;
    sysEx.append(&footer, 1);
    return sysEx;
}

void writeSfzFile(const juce::File& sfzFile, 
                  const juce::String& patchName, 
                  const juce::String& patchIndexStr,
                  const SamplingConfig& config)
{
    juce::StringArray sfzLines;
    sfzLines.add("// SFZ Mapping for " + patchName);
    sfzLines.add("<group>");
    sfzLines.add("loop_mode=no_loop");
    sfzLines.add("");

    size_t numNotes = config.midiNotes.size();
    size_t numVels = config.velocities.size();

    for (size_t n = 0; n < numNotes; ++n)
    {
        int rootNote = config.midiNotes[n];
        int loKey = (n == 0) ? 0 : (config.midiNotes[n - 1] + rootNote) / 2 + 1;
        int hiKey = (n == numNotes - 1) ? 127 : (rootNote + config.midiNotes[n + 1]) / 2;

        for (size_t v = 0; v < numVels; ++v)
        {
            int vel = config.velocities[v];
            int loVel = (v == 0) ? 0 : (config.velocities[v - 1] + vel) / 2 + 1;
            int hiVel = (v == numVels - 1) ? 127 : (vel + config.velocities[v + 1]) / 2;

            float primaryGate = config.gateLengthsSec.back();
            juce::String gateLabel = (primaryGate < 1.0f) ? juce::String(static_cast<int>(primaryGate * 1000)) + "ms" 
                                                           : juce::String(primaryGate, 1) + "s";

            juce::String samplePath = patchIndexStr + "_" + patchName 
                                    + "/" + patchIndexStr + "_" + patchName 
                                    + "_N" + juce::String(rootNote) 
                                    + "_V" + juce::String(vel) 
                                    + "_G" + gateLabel + ".wav";

            sfzLines.add("<region>");
            sfzLines.add("sample=" + samplePath);
            sfzLines.add("pitch_keycenter=" + juce::String(rootNote));
            sfzLines.add("lokey=" + juce::String(loKey) + " hikey=" + juce::String(hiKey));
            sfzLines.add("lovel=" + juce::String(loVel) + " hivel=" + juce::String(hiVel));
            sfzLines.add("");
        }
    }

    sfzFile.deleteFile();
    sfzFile.replaceWithText(sfzLines.joinIntoString("\n"));
}

void renderSample(juce::AudioProcessor& plugin, 
                  const juce::MemoryBlock& bankData, 
                  int patchIndex, 
                  int midiNote, 
                  int velocity, 
                  float gateSec, 
                  const SamplingConfig& config, 
                  const juce::File& outputFile)
{
    // Disables denormal floats during silence/tails to maintain CPU performance
    juce::ScopedNoDenormals noDenormals;

    int blockSize = 512;
    int maxTotalSamples = static_cast<int>((gateSec + config.maxReleaseSec) * config.sampleRate);
    int gateSamples = static_cast<int>(gateSec * config.sampleRate);
    float silenceGainThreshold = juce::Decibels::decibelsToGain(config.silenceThresholdDb);

    // Reset internal DSP state, delay buffers, and active voices
    plugin.reset();
    plugin.prepareToPlay(config.sampleRate, blockSize);

    juce::AudioBuffer<float> tempBuffer(2, maxTotalSamples);
    tempBuffer.clear();

    // 1. Warmup / Program Change block
    juce::MidiBuffer setupMidi;
    if (bankData.getSize() >= 4104)
        setupMidi.addEvent(juce::MidiMessage(bankData.getData(), static_cast<int>(bankData.getSize())), 0);
    
    setupMidi.addEvent(juce::MidiMessage::programChange(1, patchIndex), 1);

    juce::AudioBuffer<float> warmupBuffer(2, blockSize);
    warmupBuffer.clear();
    plugin.processBlock(warmupBuffer, setupMidi);

    // 2. Note rendering
    juce::MidiBuffer noteMidi;
    noteMidi.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<uint8_t>(velocity)), 0);

    int noteOffFrame = gateSamples;
    int samplesProcessed = 0;
    int silentBlockCount = 0;
    int actualRenderedSamples = 0;

    while (samplesProcessed < maxTotalSamples)
    {
        int numToProcess = juce::jmin(blockSize, maxTotalSamples - samplesProcessed);
        juce::MidiBuffer blockMidi;

        for (const auto metadata : noteMidi)
        {
            if (metadata.samplePosition >= samplesProcessed && metadata.samplePosition < samplesProcessed + numToProcess)
                blockMidi.addEvent(metadata.getMessage(), metadata.samplePosition - samplesProcessed);
        }

        if (noteOffFrame >= samplesProcessed && noteOffFrame < samplesProcessed + numToProcess)
            blockMidi.addEvent(juce::MidiMessage::noteOff(1, midiNote), noteOffFrame - samplesProcessed);

        juce::AudioBuffer<float> blockBuffer(tempBuffer.getArrayOfWritePointers(), 2, samplesProcessed, numToProcess);
        plugin.processBlock(blockBuffer, blockMidi);

        samplesProcessed += numToProcess;
        actualRenderedSamples = samplesProcessed;

        if (samplesProcessed > gateSamples)
        {
            float peak = blockBuffer.getMagnitude(0, numToProcess);
            if (blockBuffer.getNumChannels() > 1)
                peak = juce::jmax(peak, blockBuffer.getMagnitude(1, numToProcess));

            if (peak < silenceGainThreshold)
            {
                if (++silentBlockCount > 5)
                    break;
            }
            else
            {
                silentBlockCount = 0;
            }
        }
    }

    plugin.releaseResources();

    juce::WavAudioFormat wavFormat;
    outputFile.deleteFile();
    std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    if (fileStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(fileStream.release(), config.sampleRate, 2, config.bitDepth, {}, 0)
        );

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(tempBuffer, 0, actualRenderedSamples);
    }
}

void processBankFile(juce::AudioProcessor& plugin, const juce::File& syxFile, const juce::File& bankOutputDir, const SamplingConfig& config)
{
    juce::MemoryBlock bankData;
    syxFile.loadFileAsData(bankData);

    if (bankData.getSize() < 4104)
        return;

    for (int i = 0; i < 32; ++i)
    {
        size_t voiceOffset = 6 + (i * 128);
        const uint8_t* voicePtr = static_cast<const uint8_t*>(bankData.getData()) + voiceOffset;
        
        juce::String patchIndexStr = juce::String(i + 1).paddedLeft('0', 2);
        juce::String patchName = getPatchName(voicePtr);
        
        juce::File patchDir = bankOutputDir.getChildFile(patchIndexStr + "_" + patchName);
        patchDir.createDirectory();

        for (int note : config.midiNotes)
        {
            for (int vel : config.velocities)
            {
                for (float gate : config.gateLengthsSec)
                {
                    juce::String gateLabel = (gate < 1.0f) ? juce::String(static_cast<int>(gate * 1000)) + "ms" 
                                                           : juce::String(gate, 1) + "s";

                    juce::String sampleFileName = patchIndexStr + "_" + patchName 
                                                + "_N" + juce::String(note) 
                                                + "_V" + juce::String(vel) 
                                                + "_G" + gateLabel + ".wav";

                    juce::File sampleFile = patchDir.getChildFile(sampleFileName);
                    
                    std::cout << "Extracting: " << sampleFile.getRelativePathFrom(bankOutputDir.getParentDirectory()).toStdString() << "\n";
                    renderSample(plugin, bankData, i, note, vel, gate, config, sampleFile);
                }
            }
        }

        if (config.generateSfz)
        {
            juce::File sfzFile = bankOutputDir.getChildFile(patchIndexStr + "_" + patchName + ".sfz");
            writeSfzFile(sfzFile, patchName, patchIndexStr, config);
        }
    }
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI initialiser;

    std::vector<std::string> posArgs;
    bool generateSfz = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--sfz")
            generateSfz = true;
        else
            posArgs.push_back(arg);
    }

    if (posArgs.size() < 3)
    {
        std::cout << "Usage: dx-extractor <path_to_vst3> <input_bank_dir> <output_sample_dir> [--sfz]\n";
        return 1;
    }

    juce::File pluginFile(posArgs[0]);
    juce::File inputDir(posArgs[1]);
    juce::File outputDir(posArgs[2]);

    SamplingConfig config;
    config.generateSfz = generateSfz;

    if (!inputDir.isDirectory())
    {
        std::cerr << "Error: Input directory does not exist: " << inputDir.getFullPathName().toStdString() << "\n";
        return 1;
    }

    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    if (formatManager.getNumFormats() == 0)
    {
        std::cerr << "Error: No plugin formats registered. Ensure JUCE_PLUGINHOST_VST3=1 is set in CMake.\n";
        return 1;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        format->findAllTypesForFile(descriptions, pluginFile.getFullPathName());
        if (!descriptions.isEmpty())
            break;
    }

    if (descriptions.isEmpty())
    {
        std::cerr << "Error: Could not scan plugin description from " << pluginFile.getFullPathName().toStdString() << "\n";
        return 1;
    }

    juce::String errorMsg;
    auto pluginInstance = formatManager.createPluginInstance(*descriptions[0], 44100.0, 512, errorMsg);

    if (pluginInstance == nullptr)
    {
        std::cerr << "Failed to load plugin: " << errorMsg.toStdString() << "\n";
        return 1;
    }

    juce::Array<juce::File> bankFiles;
    inputDir.findChildFiles(bankFiles, juce::File::findFiles, true, "*.syx;*.SYX;*.mid;*.MID");

    for (const auto& syxFile : bankFiles)
    {
        juce::String relativePath = syxFile.getParentDirectory().getRelativePathFrom(inputDir);
        juce::File targetBankDir = outputDir.getChildFile(relativePath).getChildFile(syxFile.getFileNameWithoutExtension());
        targetBankDir.createDirectory();

        processBankFile(*pluginInstance, syxFile, targetBankDir, config);
    }

    std::cout << "Batch extraction complete.\n";
    return 0;
}