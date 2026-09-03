#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RGBlueDelayAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit RGBlueDelayAudioProcessorEditor(
        RGBlueDelayAudioProcessor&);

    ~RGBlueDelayAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    RGBlueDelayAudioProcessor& audioProcessor;

    juce::Slider delaySlider;
    juce::Slider repeatSlider;
    juce::Slider mixSlider;

    juce::Label delayLabel;
    juce::Label repeatLabel;
    juce::Label mixLabel;

    juce::TextButton footswitchButton;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        delayAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        repeatAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        bypassAttachment;

    void setupKnob(
        juce::Slider& slider,
        juce::Label& label,
        const juce::String& text);

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RGBlueDelayAudioProcessorEditor)
};
