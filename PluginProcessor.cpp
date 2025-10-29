#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MixCompressorAudioProcessor::MixCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

MixCompressorAudioProcessor::~MixCompressorAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MixCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Preset selector
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "preset", "Preset",
        juce::StringArray{ "Manual", "Vocal Leveler", "Drum Punch", "Bass Control", "Mix Bus Glue", "Parallel Comp" },
        0));

    // Stage 1 (Leveler) parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "threshold1", "Threshold 1",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ratio1", "Ratio 1",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 2.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + ":1"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack1", "Attack 1",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 15.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release1", "Release 1",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 200.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Stage 2 (Peak Catcher) parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "dualStage", "Dual Stage", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "threshold2", "Threshold 2",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -12.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ratio2", "Ratio 2",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 8.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + ":1"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack2", "Attack 2",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release2", "Release 2",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.4f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Global parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "makeup", "Makeup Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "autoMakeup", "Auto Makeup", true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " %"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "knee", "Knee",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 3.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return layout;
}

//==============================================================================
void MixCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    stage1.prepare(sampleRate);
    stage2.prepare(sampleRate);

    inputRMS = 0.0f;
    outputRMS = 0.0f;
}

void MixCompressorAudioProcessor::releaseResources()
{
    stage1.reset();
    stage2.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MixCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void MixCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameters
    auto threshold1 = apvts.getRawParameterValue("threshold1")->load();
    auto ratio1 = apvts.getRawParameterValue("ratio1")->load();
    auto attack1 = apvts.getRawParameterValue("attack1")->load();
    auto release1 = apvts.getRawParameterValue("release1")->load();
    auto knee = apvts.getRawParameterValue("knee")->load();

    auto dualStage = apvts.getRawParameterValue("dualStage")->load() > 0.5f;
    auto threshold2 = apvts.getRawParameterValue("threshold2")->load();
    auto ratio2 = apvts.getRawParameterValue("ratio2")->load();
    auto attack2 = apvts.getRawParameterValue("attack2")->load();
    auto release2 = apvts.getRawParameterValue("release2")->load();

    auto makeupDB = apvts.getRawParameterValue("makeup")->load();
    auto autoMakeup = apvts.getRawParameterValue("autoMakeup")->load() > 0.5f;
    auto mixPercent = apvts.getRawParameterValue("mix")->load();

    // Set compressor parameters
    stage1.setParameters(threshold1, ratio1, attack1, release1, knee);
    stage2.setParameters(threshold2, ratio2, attack2, release2, knee);

    // Create dry buffer for parallel processing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    float maxGR = 0.0f;
    float sumInputSq = 0.0f;
    float sumOutputSq = 0.0f;

    // Process audio
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float input = channelData[i];
            sumInputSq += input * input;

            // Stage 1: Leveler
            float gr1 = 0.0f;
            float output = stage1.processSample(input, gr1);

            // Stage 2: Peak Catcher (if enabled)
            float gr2 = 0.0f;
            if (dualStage)
                output = stage2.processSample(output, gr2);

            float totalGR = gr1 + gr2;
            maxGR = juce::jmax(maxGR, totalGR);

            channelData[i] = output;
            sumOutputSq += output * output;
        }
    }

    // Update RMS values
    int numSamples = buffer.getNumSamples() * totalNumInputChannels;
    float instantInputRMS = std::sqrt(sumInputSq / numSamples);
    float instantOutputRMS = std::sqrt(sumOutputSq / numSamples);

    inputRMS = rmsAlpha * inputRMS + (1.0f - rmsAlpha) * instantInputRMS;
    outputRMS = rmsAlpha * outputRMS + (1.0f - rmsAlpha) * instantOutputRMS;

    // Apply makeup gain
    float makeupGain = juce::Decibels::decibelsToGain(makeupDB);

    if (autoMakeup && maxGR > 0.01f)
    {
        float autoMakeupDB = calculateAutoMakeup(maxGR);
        makeupGain = juce::Decibels::decibelsToGain(autoMakeupDB);
    }

    // Apply mix (parallel compression)
    float wetMix = mixPercent / 100.0f;
    float dryMix = 1.0f - wetMix;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* wetData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float wet = wetData[i] * makeupGain;
            float dry = dryData[i];
            wetData[i] = wet * wetMix + dry * dryMix;
        }
    }

    // Update gain reduction meter
    currentGainReduction.store(maxGR);
}

//==============================================================================
float MixCompressorAudioProcessor::calculateAutoMakeup(float avgGainReduction)
{
    // Compensate for average gain reduction with slight headroom
    return avgGainReduction * 0.8f;
}

//==============================================================================
void MixCompressorAudioProcessor::loadPreset(PresetMode preset)
{
    auto* presetParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("preset"));
    if (presetParam)
        *presetParam = static_cast<int>(preset);

    switch (preset)
    {
    case PresetMode::VocalLeveler:
        *apvts.getRawParameterValue("threshold1") = -18.0f;
        *apvts.getRawParameterValue("ratio1") = 2.5f;
        *apvts.getRawParameterValue("attack1") = 15.0f;
        *apvts.getRawParameterValue("release1") = 150.0f;
        *apvts.getRawParameterValue("dualStage") = 0.0f;
        *apvts.getRawParameterValue("autoMakeup") = 1.0f;
        *apvts.getRawParameterValue("mix") = 100.0f;
        *apvts.getRawParameterValue("knee") = 6.0f;
        break;

    case PresetMode::DrumPunch:
        *apvts.getRawParameterValue("threshold1") = -15.0f;
        *apvts.getRawParameterValue("ratio1") = 4.0f;
        *apvts.getRawParameterValue("attack1") = 25.0f;
        *apvts.getRawParameterValue("release1") = 100.0f;
        *apvts.getRawParameterValue("dualStage") = 0.0f;
        *apvts.getRawParameterValue("autoMakeup") = 1.0f;
        *apvts.getRawParameterValue("mix") = 100.0f;
        *apvts.getRawParameterValue("knee") = 3.0f;
        break;

    case PresetMode::BassControl:
        *apvts.getRawParameterValue("threshold1") = -20.0f;
        *apvts.getRawParameterValue("ratio1") = 4.0f;
        *apvts.getRawParameterValue("attack1") = 5.0f;
        *apvts.getRawParameterValue("release1") = 200.0f;
        *apvts.getRawParameterValue("dualStage") = 0.0f;
        *apvts.getRawParameterValue("autoMakeup") = 1.0f;
        *apvts.getRawParameterValue("mix") = 100.0f;
        *apvts.getRawParameterValue("knee") = 4.0f;
        break;

    case PresetMode::MixBusGlue:
        *apvts.getRawParameterValue("threshold1") = -10.0f;
        *apvts.getRawParameterValue("ratio1") = 2.0f;
        *apvts.getRawParameterValue("attack1") = 30.0f;
        *apvts.getRawParameterValue("release1") = 300.0f;
        *apvts.getRawParameterValue("dualStage") = 0.0f;
        *apvts.getRawParameterValue("autoMakeup") = 1.0f;
        *apvts.getRawParameterValue("mix") = 100.0f;
        *apvts.getRawParameterValue("knee") = 3.0f;
        break;

    case PresetMode::ParallelComp:
        *apvts.getRawParameterValue("threshold1") = -25.0f;
        *apvts.getRawParameterValue("ratio1") = 6.0f;
        *apvts.getRawParameterValue("attack1") = 10.0f;
        *apvts.getRawParameterValue("release1") = 120.0f;
        *apvts.getRawParameterValue("dualStage") = 1.0f;
        *apvts.getRawParameterValue("threshold2") = -10.0f;
        *apvts.getRawParameterValue("ratio2") = 10.0f;
        *apvts.getRawParameterValue("attack2") = 2.0f;
        *apvts.getRawParameterValue("release2") = 50.0f;
        *apvts.getRawParameterValue("autoMakeup") = 1.0f;
        *apvts.getRawParameterValue("mix") = 30.0f;
        *apvts.getRawParameterValue("knee") = 6.0f;
        break;

    default:
        break;
    }
}

//==============================================================================
// Compressor Stage Implementation
void MixCompressorAudioProcessor::CompressorStage::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void MixCompressorAudioProcessor::CompressorStage::setParameters(float threshold, float r, float attack, float release, float knee)
{
    thresholdDB = threshold;
    ratio = r;
    kneeWidth = knee;

    // Convert attack/release times to coefficients
    attackCoef = 1.0f - std::exp(-1.0f / (attack * 0.001f * static_cast<float>(sampleRate)));
    releaseCoef = 1.0f - std::exp(-1.0f / (release * 0.001f * static_cast<float>(sampleRate)));
}

float MixCompressorAudioProcessor::CompressorStage::processSample(float input, float& grOut)
{
    float inputAbs = std::fabs(input);

    // Envelope follower
    if (inputAbs > envelopeFollower)
        envelopeFollower += (inputAbs - envelopeFollower) * attackCoef;
    else
        envelopeFollower += (inputAbs - envelopeFollower) * releaseCoef;

    // Convert to dB
    float envDB = juce::Decibels::gainToDecibels(envelopeFollower + 0.0001f);

    // Apply compression curve
    float gainReductionDB = applyCompressionCurve(envDB);
    grOut = gainReductionDB;

    // Convert back to linear gain
    float gain = juce::Decibels::decibelsToGain(-gainReductionDB);

    return input * gain;
}

float MixCompressorAudioProcessor::CompressorStage::applyCompressionCurve(float inputDB)
{
    float overThreshold = inputDB - thresholdDB;

    if (overThreshold <= -kneeWidth * 0.5f)
    {
        // Below knee
        return 0.0f;
    }
    else if (overThreshold >= kneeWidth * 0.5f)
    {
        // Above knee - full compression
        return overThreshold * (1.0f - 1.0f / ratio);
    }
    else
    {
        // In knee - soft transition
        float kneeInput = overThreshold + kneeWidth * 0.5f;
        float kneeFactor = (kneeInput * kneeInput) / (2.0f * kneeWidth);
        return kneeFactor * (1.0f - 1.0f / ratio);
    }
}

void MixCompressorAudioProcessor::CompressorStage::reset()
{
    envelopeFollower = 0.0f;
}

//==============================================================================
bool MixCompressorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MixCompressorAudioProcessor::createEditor()
{
    return new MixCompressorAudioProcessorEditor(*this);
}

//==============================================================================
void MixCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MixCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
const juce::String MixCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MixCompressorAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MixCompressorAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool MixCompressorAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MixCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MixCompressorAudioProcessor::getNumPrograms()
{
    return 1;
}

int MixCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MixCompressorAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String MixCompressorAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void MixCompressorAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixCompressorAudioProcessor();
}