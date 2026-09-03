#pragma once

#include <JuceHeader.h>

//==============================================================
// RG BLUE DELAY
// Blue Ocean / Deep Blue inspired delay DSP
// 3 controls: DELAY / REPEAT / MIX
//==============================================================

class RGBlueDelayAudioProcessor : public juce::AudioProcessor
{
public:

    RGBlueDelayAudioProcessor();
    ~RGBlueDelayAudioProcessor() override;

    //==========================================================
    // AudioProcessor
    //==========================================================

    void prepareToPlay(double sampleRate,
                       int samplesPerBlock) override;

    void releaseResources() override;

    bool isBusesLayoutSupported(
        const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    //==========================================================
    // Editor
    //==========================================================

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==========================================================
    // Plugin information
    //==========================================================

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    //==========================================================
    // Programs
    //==========================================================

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;
    void changeProgramName(
        int index,
        const juce::String& newName) override;

    //==========================================================
    // State
    //==========================================================

    void getStateInformation(
        juce::MemoryBlock& destData) override;

    void setStateInformation(
        const void* data,
        int sizeInBytes) override;

    //==========================================================
    // Parameters
    //==========================================================

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

private:

    //==========================================================
    // Component-inspired DSP
    //==========================================================

    juce::dsp::IIR::Filter<float> inputHighPass;
    juce::dsp::IIR::Filter<float> inputLowPass;

    juce::dsp::IIR::Filter<float> feedbackHighPass;
    juce::dsp::IIR::Filter<float> feedbackLowPass;

    juce::dsp::IIR::Filter<float> outputLowPass;

    //==========================================================
    // PT2399-style delay line
    //==========================================================

    juce::AudioBuffer<float> delayBuffer;

    int delayBufferSize = 0;
    int writePosition = 0;

    float currentDelaySamples = 0.0f;

    //==========================================================
    // Smoothed parameters
    //==========================================================

    juce::SmoothedValue<float> delaySmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    juce::SmoothedValue<float> bypassSmoothed;

    //==========================================================
    // Internal state
    //==========================================================

    double currentSampleRate = 44100.0;

    bool bypassState = false;

    //==========================================================
    // DSP helpers
    //==========================================================

    float readDelaySample(
        int channel,
        float delaySamples) const;

    void updateFilters();

    //==========================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RGBlueDelayAudioProcessor)
};
