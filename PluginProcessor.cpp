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
    makeupGainSmoothed.reset(44100.0, 0.05); // 50ms smoothing for makeup gain
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

    makeupGainSmoothed.reset(sampleRate, 0.05);
    makeupGainSmoothed.setCurrentAndTargetValue(1.0f);

    inputRMS = 0.0f;
    outputRMS = 0.0f;

    // Reset DC blocker
    for (int i = 0; i < 2; ++i)
    {
        dcBlockerX1[i] = 0.0f;
        dcBlockerY1[i] = 0.0f;
    }
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

    // Get parameters safely
    auto* threshold1Param = apvts.getRawParameterValue("threshold1");
    auto* ratio1Param = apvts.getRawParameterValue("ratio1");
    auto* attack1Param = apvts.getRawParameterValue("attack1");
    auto* release1Param = apvts.getRawParameterValue("release1");
    auto* kneeParam = apvts.getRawParameterValue("knee");
    auto* dualStageParam = apvts.getRawParameterValue("dualStage");
    auto* threshold2Param = apvts.getRawParameterValue("threshold2");
    auto* ratio2Param = apvts.getRawParameterValue("ratio2");
    auto* attack2Param = apvts.getRawParameterValue("attack2");
    auto* release2Param = apvts.getRawParameterValue("release2");
    auto* makeupParam = apvts.getRawParameterValue("makeup");
    auto* autoMakeupParam = apvts.getRawParameterValue("autoMakeup");
    auto* mixParam = apvts.getRawParameterValue("mix");

    if (!threshold1Param || !ratio1Param || !attack1Param || !release1Param || !kneeParam ||
        !dualStageParam || !threshold2Param || !ratio2Param || !attack2Param || !release2Param ||
        !makeupParam || !autoMakeupParam || !mixParam)
        return;

    auto threshold1 = threshold1Param->load();
    auto ratio1 = ratio1Param->load();
    auto attack1 = attack1Param->load();
    auto release1 = release1Param->load();
    auto knee = kneeParam->load();

    auto dualStage = dualStageParam->load() > 0.5f;
    auto threshold2 = threshold2Param->load();
    auto ratio2 = ratio2Param->load();
    auto attack2 = attack2Param->load();
    auto release2 = release2Param->load();

    auto makeupDB = makeupParam->load();
    auto autoMakeup = autoMakeupParam->load() > 0.5f;
    auto mixPercent = mixParam->load();

    // Set compressor parameters
    stage1.setParameters(threshold1, ratio1, attack1, release1, knee);
    stage2.setParameters(threshold2, ratio2, attack2, release2, knee);

    // Create dry buffer for parallel processing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    float maxGR = 0.0f;
    float sumInputSq = 0.0f;
    float sumOutputSq = 0.0f;

    // Process audio sample-by-sample for smooth gain changes
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float input = channelData[i];

            // DC blocker to prevent offset issues
            float dcBlocked = input - dcBlockerX1[channel] + dcBlockerCoef * dcBlockerY1[channel];
            dcBlockerX1[channel] = input;
            dcBlockerY1[channel] = dcBlocked;
            input = dcBlocked;

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
    if (numSamples > 0)
    {
        float instantInputRMS = std::sqrt(sumInputSq / numSamples);
        float instantOutputRMS = std::sqrt(sumOutputSq / numSamples);

        inputRMS = rmsAlpha * inputRMS + (1.0f - rmsAlpha) * instantInputRMS;
        outputRMS = rmsAlpha * outputRMS + (1.0f - rmsAlpha) * instantOutputRMS;
    }

    // Calculate and smooth makeup gain
    float targetMakeupGain = juce::Decibels::decibelsToGain(makeupDB);

    if (autoMakeup && maxGR > 0.01f)
    {
        float autoMakeupDB = calculateAutoMakeup(maxGR);
        targetMakeupGain = juce::Decibels::decibelsToGain(autoMakeupDB);
    }

    makeupGainSmoothed.setTargetValue(targetMakeupGain);

    // Apply mix (parallel compression) with smoothed makeup gain
    float wetMix = mixPercent / 100.0f;
    float dryMix = 1.0f - wetMix;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* wetData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float makeupGain = makeupGainSmoothed.getNextValue();
            float wet = wetData[i] * makeupGain;
            float dry = dryData[i];
            wetData[i] = wet * wetMix + dry * dryMix;

            // Soft clip to prevent any possible overshoot
            wetData[i] = std::tanh(wetData[i] * 0.9f) / 0.9f;
        }
    }

    // Update gain reduction meter
    currentGainReduction.store(maxGR);
}

//==============================================================================
float MixCompressorAudioProcessor::calculateAutoMakeup(float avgGainReduction)
{
    // Compensate for average gain reduction with slight headroom
    return avgGainReduction * 0.75f;
}

//==============================================================================
void MixCompressorAudioProcessor::loadPreset(PresetMode preset)
{
    auto* presetParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("preset"));
    if (presetParam)
        *presetParam = static_cast<int>(preset);

    auto* threshold1Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("threshold1"));
    auto* ratio1Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("ratio1"));
    auto* attack1Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("attack1"));
    auto* release1Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("release1"));
    auto* dualStageParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("dualStage"));
    auto* threshold2Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("threshold2"));
    auto* ratio2Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("ratio2"));
    auto* attack2Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("attack2"));
    auto* release2Param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("release2"));
    auto* autoMakeupParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("autoMakeup"));
    auto* mixParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("mix"));
    auto* kneeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("knee"));

    if (!threshold1Param || !ratio1Param || !attack1Param || !release1Param || !dualStageParam ||
        !threshold2Param || !ratio2Param || !attack2Param || !release2Param || !autoMakeupParam ||
        !mixParam || !kneeParam)
        return;

    switch (preset)
    {
    case PresetMode::VocalLeveler:
        *threshold1Param = -18.0f;
        *ratio1Param = 2.5f;
        *attack1Param = 15.0f;
        *release1Param = 150.0f;
        *dualStageParam = false;
        *autoMakeupParam = true;
        *mixParam = 100.0f;
        *kneeParam = 6.0f;
        break;

    case PresetMode::DrumPunch:
        *threshold1Param = -15.0f;
        *ratio1Param = 4.0f;
        *attack1Param = 25.0f;
        *release1Param = 100.0f;
        *dualStageParam = false;
        *autoMakeupParam = true;
        *mixParam = 100.0f;
        *kneeParam = 3.0f;
        break;

    case PresetMode::BassControl:
        *threshold1Param = -20.0f;
        *ratio1Param = 4.0f;
        *attack1Param = 5.0f;
        *release1Param = 200.0f;
        *dualStageParam = false;
        *autoMakeupParam = true;
        *mixParam = 100.0f;
        *kneeParam = 4.0f;
        break;

    case PresetMode::MixBusGlue:
        *threshold1Param = -10.0f;
        *ratio1Param = 2.0f;
        *attack1Param = 30.0f;
        *release1Param = 300.0f;
        *dualStageParam = false;
        *autoMakeupParam = true;
        *mixParam = 100.0f;
        *kneeParam = 3.0f;
        break;

    case PresetMode::ParallelComp:
        *threshold1Param = -25.0f;
        *ratio1Param = 6.0f;
        *attack1Param = 10.0f;
        *release1Param = 120.0f;
        *dualStageParam = true;
        *threshold2Param = -10.0f;
        *ratio2Param = 10.0f;
        *attack2Param = 2.0f;
        *release2Param = 50.0f;
        *autoMakeupParam = true;
        *mixParam = 30.0f;
        *kneeParam = 6.0f;
        break;

    default:
        break;
    }
}

//==============================================================================
// Compressor Stage Implementation with improved smoothing
void MixCompressorAudioProcessor::CompressorStage::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void MixCompressorAudioProcessor::CompressorStage::setParameters(float threshold, float r, float attack, float release, float knee)
{
    thresholdDB = threshold;
    ratio = juce::jmax(1.0f, r); // Ensure ratio is at least 1:1
    kneeWidth = knee;

    // Convert attack/release times to coefficients with minimum values to prevent instability
    float attackMs = juce::jmax(0.1f, attack);
    float releaseMs = juce::jmax(20.0f, release);

    attackCoef = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)));
    releaseCoef = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

    // Clamp coefficients to safe range
    attackCoef = juce::jlimit(0.0001f, 0.9999f, attackCoef);
    releaseCoef = juce::jlimit(0.0001f, 0.9999f, releaseCoef);
}

float MixCompressorAudioProcessor::CompressorStage::processSample(float input, float& grOut)
{
    // Use absolute value for peak detection
    float inputAbs = std::fabs(input);

    // Peak envelope follower with proper ballistics
    if (inputAbs > peakEnvelope)
        peakEnvelope += (inputAbs - peakEnvelope) * attackCoef;
    else
        peakEnvelope += (inputAbs - peakEnvelope) * releaseCoef;

    // Clamp envelope to prevent extreme values
    peakEnvelope = juce::jlimit(0.0f, 10.0f, peakEnvelope);

    // Convert to dB with safe floor
    float envDB = juce::Decibels::gainToDecibels(peakEnvelope + 1e-6f);

    // Apply compression curve
    float gainReductionDB = applyCompressionCurve(envDB);
    grOut = gainReductionDB;

    // Convert back to linear gain
    float targetGain = juce::Decibels::decibelsToGain(-gainReductionDB);

    // Smooth the gain changes to prevent clicks
    gainSmooth += (targetGain - gainSmooth) * gainSmoothingCoef;
    gainSmooth = juce::jlimit(0.01f, 1.0f, gainSmooth);

    return input * gainSmooth;
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
        float grDB = overThreshold * (1.0f - 1.0f / ratio);
        return juce::jlimit(0.0f, 60.0f, grDB); // Clamp to reasonable range
    }
    else
    {
        // In knee - soft transition using quadratic curve
        float kneeInput = overThreshold + kneeWidth * 0.5f;
        float kneeFactor = (kneeInput * kneeInput) / (2.0f * kneeWidth);
        float grDB = kneeFactor * (1.0f - 1.0f / ratio);
        return juce::jlimit(0.0f, 60.0f, grDB);
    }
}

void MixCompressorAudioProcessor::CompressorStage::reset()
{
    peakEnvelope = 0.0f;
    gainSmooth = 1.0f;
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