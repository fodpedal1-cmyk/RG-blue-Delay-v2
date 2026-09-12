#pragma once

#include <JuceHeader.h>

class RGBlueDelayAudioProcessorEditor;

class RGBlueDelayAudioProcessor : public juce::AudioProcessor
{
public:
    RGBlueDelayAudioProcessor();
    ~RGBlueDelayAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>&,
        juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;
    void changeProgramName(
        int index,
        const juce::String& newName) override;

    void getStateInformation(
        juce::MemoryBlock& destData) override;

    void setStateInformation(
        const void* data,
        int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    struct RCFilter
    {
        float a = 0.0f;
        float b = 0.0f;
        float z = 0.0f;

        void reset()
        {
            z = 0.0f;
        }

        void setLowPass(
            double sampleRate,
            float cutoff);

        void setHighPass(
            double sampleRate,
            float cutoff);

        float process(float input);
    };

    struct DelayLine
    {
        juce::AudioBuffer<float> buffer;
        int size = 0;
        int writePosition = 0;

        void prepare(
            int channels,
            int samples);

        void clear();

        void write(
            int channel,
            float value);

        float read(
            int channel,
            float delaySamples) const;

        void advance();
    };

    struct ChannelState
    {
        RCFilter inputHP;
        RCFilter inputLP;

        RCFilter delayHP;
        RCFilter delayLP;

        RCFilter feedbackHP;
        RCFilter feedbackLP;

        RCFilter outputLP;

        float feedbackState = 0.0f;
        float delayClock = 0.0f;

        void reset();
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    void resetDSP();

    float processDelaySample(
        int channel,
        float input,
        float delaySamples,
        float feedback,
        float mix);

    float readDelaySample(
        int channel,
        float delaySamples) const;

    void updateParameters();

    juce::AudioBuffer<float> delayBuffer;

    ChannelState channelState[2];

    juce::SmoothedValue<float> delaySmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> bypassSmoothed;

    double currentSampleRate = 44100.0;

    int delayBufferSize = 0;
    int writePosition = 0;

    float delayTimeMs = 300.0f;
    float feedbackAmount = 0.45f;
    float mixAmount = 0.32f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        RGBlueDelayAudioProcessor)
};
