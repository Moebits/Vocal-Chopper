#pragma once
#include <JuceHeader.h>

class Processor : public AudioProcessor {
public:
    Processor() : AudioProcessor() {}
    ~Processor() override {}
    
    auto prepareToPlay([[maybe_unused]] double sampleRate, [[maybe_unused]] int samplesPerBlock) -> void override {}
    auto releaseResources() -> void override {}
    auto processBlock(AudioBuffer<float>&, MidiBuffer&) -> void override {}

    auto isBusesLayoutSupported ([[maybe_unused]] const BusesLayout& layouts) const -> bool override { return true; }
    auto createEditor() -> AudioProcessorEditor* override;

    inline auto hasEditor() const -> bool override { return true; }
    inline auto getName() const -> const String override { return JucePlugin_Name; }
    inline auto acceptsMidi() const -> bool override { return false; }
    inline auto producesMidi() const -> bool override { return false; }
    inline auto isMidiEffect() const -> bool override { return false; }
    inline auto getTailLengthSeconds() const -> double override { return 0.0; }

    auto getNumPrograms() -> int override { return 0; }
    auto getCurrentProgram() -> int override { return 0; }
    auto setCurrentProgram([[maybe_unused]] int index) -> void override {}
    auto getProgramName([[maybe_unused]] int index) -> const String override { return ""; }
    auto changeProgramName([[maybe_unused]] int index, [[maybe_unused]] const String& newName) -> void override {}

    auto getStateInformation([[maybe_unused]] MemoryBlock& destData) -> void override;
    auto setStateInformation([[maybe_unused]] const void* data, [[maybe_unused]] int sizeInBytes) -> void override;

    String audioPath = "";
    String destFolder = "";
    bool skipVocalExtraction = false;
    bool keepVocalFile = false;
    MemoryBlock droppedFileBytes;
    String state = "";
    double progress = 100.0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};