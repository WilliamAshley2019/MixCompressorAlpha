#pragma once

#include <JuceHeader.h>

//==============================================================================
class MixCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MixCompressorAudioProcessor();
    ~MixCompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Preset system
    enum class PresetMode
    {
        Manual = 0,
        VocalLeveler,
        DrumPunch,
        BassControl,
        MixBusGlue,
        ParallelComp,
        NumPresets
    };

    void loadPreset(PresetMode preset);
    float getCurrentGainReduction() const { return currentGainReduction; }

    // Parameter access
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

private:
    //==============================================================================
    // Compressor engine - single stage
    class CompressorStage
    {
    public:
        void prepare(double sampleRate);
        void setParameters(float threshold, float ratio, float attack, float release, float knee);
        float processSample(float input, float& grOut);
        void reset();

    private:
        float envelopeFollower = 0.0f;
        float attackCoef = 0.0f;
        float releaseCoef = 0.0f;
        float thresholdDB = -24.0f;
        float ratio = 4.0f;
        float kneeWidth = 6.0f;
        double sampleRate = 44100.0;

        float applyCompressionCurve(float inputDB);
    };

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Dual-stage compression
    CompressorStage stage1; // Leveler
    CompressorStage stage2; // Peak catcher

    // Metering
    std::atomic<float> currentGainReduction{ 0.0f };

    // Auto makeup gain calculation
    float calculateAutoMakeup(float avgGainReduction);

    // RMS calculation for level matching
    float inputRMS = 0.0f;
    float outputRMS = 0.0f;
    static constexpr float rmsAlpha = 0.99f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixCompressorAudioProcessor)
};