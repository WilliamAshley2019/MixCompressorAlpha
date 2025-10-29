#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MixCompressorAudioProcessorEditor::MixCompressorAudioProcessorEditor(MixCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Color scheme
    backgroundColour = juce::Colour(0xff1a1a1a);
    panelColour = juce::Colour(0xff2d2d2d);
    accentColour = juce::Colour(0xff4a9eff);

    // Preset selector
    presetSelector.addItem("Manual", 1);
    presetSelector.addItem("Vocal Leveler", 2);
    presetSelector.addItem("Drum Punch", 3);
    presetSelector.addItem("Bass Control", 4);
    presetSelector.addItem("Mix Bus Glue", 5);
    presetSelector.addItem("Parallel Comp", 6);
    presetSelector.setSelectedId(1);
    presetSelector.onChange = [this]
        {
            auto preset = static_cast<MixCompressorAudioProcessor::PresetMode>(presetSelector.getSelectedId() - 1);
            audioProcessor.loadPreset(preset);
        };
    addAndMakeVisible(presetSelector);
    presetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "preset", presetSelector);

    // Stage 1 controls
    setupRotarySlider(threshold1Slider);
    setupRotarySlider(ratio1Slider);
    setupRotarySlider(attack1Slider);
    setupRotarySlider(release1Slider);

    setupLabel(threshold1Label, "THRESHOLD");
    setupLabel(ratio1Label, "RATIO");
    setupLabel(attack1Label, "ATTACK");
    setupLabel(release1Label, "RELEASE");

    threshold1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "threshold1", threshold1Slider);
    ratio1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "ratio1", ratio1Slider);
    attack1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "attack1", attack1Slider);
    release1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "release1", release1Slider);

    // Dual stage toggle
    dualStageToggle.setButtonText("Dual Stage");
    dualStageToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    dualStageToggle.setColour(juce::ToggleButton::tickColourId, accentColour);
    addAndMakeVisible(dualStageToggle);
    dualStageAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "dualStage", dualStageToggle);

    // Stage 2 controls
    setupRotarySlider(threshold2Slider);
    setupRotarySlider(ratio2Slider);
    setupRotarySlider(attack2Slider);
    setupRotarySlider(release2Slider);

    setupLabel(threshold2Label, "THR 2");
    setupLabel(ratio2Label, "RATIO 2");
    setupLabel(attack2Label, "ATK 2");
    setupLabel(release2Label, "REL 2");

    threshold2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "threshold2", threshold2Slider);
    ratio2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "ratio2", ratio2Slider);
    attack2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "attack2", attack2Slider);
    release2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "release2", release2Slider);

    // Global controls
    setupRotarySlider(makeupSlider);
    setupRotarySlider(mixSlider);
    setupRotarySlider(kneeSlider);

    setupLabel(makeupLabel, "MAKEUP");
    setupLabel(mixLabel, "MIX");
    setupLabel(kneeLabel, "KNEE");

    makeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "makeup", makeupSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "mix", mixSlider);
    kneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "knee", kneeSlider);

    autoMakeupToggle.setButtonText("Auto Makeup");
    autoMakeupToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    autoMakeupToggle.setColour(juce::ToggleButton::tickColourId, accentColour);
    addAndMakeVisible(autoMakeupToggle);
    autoMakeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "autoMakeup", autoMakeupToggle);

    // Gain reduction meter
    addAndMakeVisible(grMeter);

    // Start timer for metering updates
    startTimerHz(30);

    setSize(800, 500);
}

MixCompressorAudioProcessorEditor::~MixCompressorAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void MixCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("MIX COMPRESSOR", 20, 10, 300, 30, juce::Justification::left);

    // Subtitle
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colours::grey);
    g.drawText("Based on Mike Senior's Mixing Principles", 20, 35, 300, 20, juce::Justification::left);

    // Panel backgrounds
    g.setColour(panelColour);
    g.fillRoundedRectangle(15, 70, 770, 180, 5);  // Stage 1
    g.fillRoundedRectangle(15, 260, 770, 180, 5); // Stage 2
    g.fillRoundedRectangle(600, 450, 185, 35, 5); // Controls

    // Section labels
    g.setColour(accentColour);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("STAGE 1 - LEVELER", 25, 75, 200, 20, juce::Justification::left);
    g.drawText("STAGE 2 - PEAK CATCHER", 25, 265, 200, 20, juce::Justification::left);

    // Info text
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(10.0f));
    g.drawText("Use Stage 1 for smooth leveling | Stage 2 for peak control",
        25, 240, 400, 15, juce::Justification::left);
}

void MixCompressorAudioProcessorEditor::resized()
{
    // Preset selector
    presetSelector.setBounds(600, 15, 185, 30);

    // Stage 1 controls
    int stage1Y = 100;
    threshold1Slider.setBounds(30, stage1Y, 100, 100);
    threshold1Label.setBounds(30, stage1Y + 105, 100, 20);

    ratio1Slider.setBounds(150, stage1Y, 100, 100);
    ratio1Label.setBounds(150, stage1Y + 105, 100, 20);

    attack1Slider.setBounds(270, stage1Y, 100, 100);
    attack1Label.setBounds(270, stage1Y + 105, 100, 20);

    release1Slider.setBounds(390, stage1Y, 100, 100);
    release1Label.setBounds(390, stage1Y + 105, 100, 20);

    // Stage 2 toggle
    dualStageToggle.setBounds(25, 290, 150, 25);

    // Stage 2 controls
    int stage2Y = 300;
    threshold2Slider.setBounds(30, stage2Y, 80, 80);
    threshold2Label.setBounds(30, stage2Y + 85, 80, 20);

    ratio2Slider.setBounds(130, stage2Y, 80, 80);
    ratio2Label.setBounds(130, stage2Y + 85, 80, 20);

    attack2Slider.setBounds(230, stage2Y, 80, 80);
    attack2Label.setBounds(230, stage2Y + 85, 80, 20);

    release2Slider.setBounds(330, stage2Y, 80, 80);
    release2Label.setBounds(330, stage2Y + 85, 80, 20);

    // Global controls
    makeupSlider.setBounds(520, stage1Y, 100, 100);
    makeupLabel.setBounds(520, stage1Y + 105, 100, 20);

    mixSlider.setBounds(640, stage1Y, 100, 100);
    mixLabel.setBounds(640, stage1Y + 105, 100, 20);

    kneeSlider.setBounds(520, stage2Y, 100, 100);
    kneeLabel.setBounds(520, stage2Y + 85, 100, 20);

    autoMakeupToggle.setBounds(640, stage2Y + 30, 140, 25);

    // Gain reduction meter
    grMeter.setBounds(15, 450, 570, 35);
}

void MixCompressorAudioProcessorEditor::timerCallback()
{
    // Update gain reduction meter
    float gr = audioProcessor.getCurrentGainReduction();
    grMeter.setGainReduction(gr);
}

//==============================================================================
void MixCompressorAudioProcessorEditor::setupRotarySlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);
}

void MixCompressorAudioProcessorEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(label);
}

//==============================================================================
void MixCompressorAudioProcessorEditor::GainReductionMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);

    // Background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(bounds, 3);

    // Label
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("GAIN REDUCTION", bounds.removeFromLeft(120), juce::Justification::centredLeft);

    // Meter area
    auto meterBounds = bounds.reduced(5, 5);

    // Scale markings
    g.setColour(juce::Colours::grey);
    g.setFont(juce::Font(9.0f));
    for (int db = 0; db >= -18; db -= 3)
    {
        float x = juce::jmap(static_cast<float>(db), -18.0f, 0.0f,
            meterBounds.getX(), meterBounds.getRight());
        g.drawLine(x, meterBounds.getY(), x, meterBounds.getBottom(), 1.0f);
        g.drawText(juce::String(db), x - 10, meterBounds.getBottom() - 12, 20, 12,
            juce::Justification::centred);
    }

    // Gain reduction bar
    if (gainReduction > 0.1f)
    {
        float grClamped = juce::jmin(gainReduction, 18.0f);
        float barWidth = juce::jmap(grClamped, 0.0f, 18.0f, 0.0f, meterBounds.getWidth());

        auto grRect = meterBounds.withWidth(barWidth);

        // Color gradient based on amount of GR
        juce::Colour meterColour;
        if (grClamped < 6.0f)
            meterColour = juce::Colour(0xff4a9eff); // Blue - gentle
        else if (grClamped < 12.0f)
            meterColour = juce::Colour(0xffffa500); // Orange - medium
        else
            meterColour = juce::Colour(0xffff4444); // Red - heavy

        g.setColour(meterColour);
        g.fillRoundedRectangle(grRect, 2);

        // Value text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(juce::String(gainReduction, 1) + " dB",
            meterBounds.withWidth(100).translated(meterBounds.getWidth() - 100, -15),
            juce::Justification::centredRight);
    }
}