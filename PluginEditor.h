#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class MixCompressorAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    MixCompressorAudioProcessorEditor(MixCompressorAudioProcessor&);
    ~MixCompressorAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    //==============================================================================
    class GainReductionMeter : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override;
        void setGainReduction(float gr) { gainReduction = gr; repaint(); }

    private:
        float gainReduction = 0.0f;
    };

    //==============================================================================
    MixCompressorAudioProcessor& audioProcessor;

    // UI Components
    juce::ComboBox presetSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAttachment;

    // Stage 1 controls
    juce::Slider threshold1Slider, ratio1Slider, attack1Slider, release1Slider;
    juce::Label threshold1Label, ratio1Label, attack1Label, release1Label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshold1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratio1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attack1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> release1Attachment;

    // Stage 2 controls
    juce::ToggleButton dualStageToggle;
    juce::Slider threshold2Slider, ratio2Slider, attack2Slider, release2Slider;
    juce::Label threshold2Label, ratio2Label, attack2Label, release2Label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dualStageAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> threshold2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratio2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attack2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> release2Attachment;

    // Global controls
    juce::Slider makeupSlider, mixSlider, kneeSlider;
    juce::Label makeupLabel, mixLabel, kneeLabel;
    juce::ToggleButton autoMakeupToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoMakeupAttachment;

    // Metering
    GainReductionMeter grMeter;

    // Styling
    juce::Colour backgroundColour;
    juce::Colour panelColour;
    juce::Colour accentColour;

    void setupRotarySlider(juce::Slider& slider);
    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixCompressorAudioProcessorEditor)
};