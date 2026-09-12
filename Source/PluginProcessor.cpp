#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

namespace
{
    constexpr float minDelayMs = 25.0f;
    constexpr float maxDelayMs = 450.0f;

    constexpr float maxFeedback = 0.92f;

    constexpr float R1  = 1000000.0f;
    constexpr float R2  = 180000.0f;
    constexpr float R3  = 360000.0f;
    constexpr float R4  = 22000.0f;
    constexpr float R5  = 12000.0f;
    constexpr float R6  = 1000.0f;
    constexpr float R7  = 10000.0f;
    constexpr float R8  = 10000.0f;
    constexpr float R9  = 10000.0f;
    constexpr float R10 = 5100.0f;
    constexpr float R11 = 20000.0f;
    constexpr float R12 = 10000.0f;
    constexpr float R13 = 1000.0f;
    constexpr float R14 = 2000.0f;
    constexpr float R15 = 20000.0f;
    constexpr float R16 = 10000.0f;
    constexpr float R17 = 2200.0f;
    constexpr float R18 = 33.0f;
    constexpr float R19 = 10000.0f;
    constexpr float R20 = 10000.0f;
    constexpr float R22 = 4700.0f;

    constexpr float C1  = 22e-9f;
    constexpr float C2  = 47e-12f;
    constexpr float C3  = 100e-12f;
    constexpr float C4  = 1e-6f;
    constexpr float C5  = 1e-6f;
    constexpr float C6  = 4.7e-9f;
    constexpr float C7  = 2.2e-9f;
    constexpr float C8  = 2.2e-9f;
    constexpr float C9  = 100e-9f;
    constexpr float C10 = 100e-9f;
    constexpr float C11 = 22e-9f;
    constexpr float C12 = 10e-9f;
    constexpr float C13 = 1e-6f;
    constexpr float C14 = 1e-6f;
    constexpr float C15 = 47e-9f;
    constexpr float C16 = 15e-9f;
    constexpr float C17 = 47e-6f;
    constexpr float C18 = 100e-9f;
    constexpr float C19 = 100e-9f;
    constexpr float C20 = 100e-6f;
    constexpr float C21 = 47e-6f;
    constexpr float C22 = 47e-6f;

    constexpr float TL072Gain =
        1.0f + (R3 / R2);

    constexpr float inputHighPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R1 * C1);

    constexpr float inputLowPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R3 * C2);

    constexpr float delayHighPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R4 * C6);

    constexpr float delayLowPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R5 * C7);

    constexpr float feedbackHighPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R11 * C11);

    constexpr float feedbackLowPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R12 * C12);

    constexpr float outputHighPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R15 * C15);

    constexpr float outputLowPassHz =
        1.0f / (2.0f *
                juce::MathConstants<float>::pi *
                R16 * C16);

    constexpr float pt2399Smoothing =
        0.18f;

    constexpr float feedbackSoftness =
        0.72f;
}

//==============================================================

void RGBlueDelayAudioProcessor::RCFilter::setLowPass(
    double sampleRate,
    float cutoff)
{
    cutoff = juce::jlimit(
        1.0f,
        static_cast<float>(sampleRate * 0.45),
        cutoff);

    const float x =
        std::exp(
            -2.0f *
            juce::MathConstants<float>::pi *
            cutoff /
            static_cast<float>(sampleRate));

    a = 1.0f - x;
    b = x;
}

void RGBlueDelayAudioProcessor::RCFilter::setHighPass(
    double sampleRate,
    float cutoff)
{
    cutoff = juce::jlimit(
        1.0f,
        static_cast<float>(sampleRate * 0.45),
        cutoff);

    const float x =
        std::exp(
            -2.0f *
            juce::MathConstants<float>::pi *
            cutoff /
            static_cast<float>(sampleRate));

    a = (1.0f + x) * 0.5f;
    b = x;
}

float RGBlueDelayAudioProcessor::RCFilter::process(
    float input)
{
    const float output =
        a * input +
        b * z;

    z = output;

    return output;
}

//==============================================================

void RGBlueDelayAudioProcessor::DelayLine::prepare(
    int channels,
    int samples)
{
    size = std::max(1, samples);

    buffer.setSize(
        channels,
        size);

    clear();
}

void RGBlueDelayAudioProcessor::DelayLine::clear()
{
    buffer.clear();
    writePosition = 0;
}

void RGBlueDelayAudioProcessor::DelayLine::write(
    int channel,
    float value)
{
    if (channel >= 0 &&
        channel < buffer.getNumChannels())
    {
        buffer.setSample(
            channel,
            writePosition,
            value);
    }
}

float RGBlueDelayAudioProcessor::DelayLine::read(
    int channel,
    float delaySamples) const
{
    if (size <= 1)
        return 0.0f;

    float position =
        static_cast<float>(writePosition)
        - delaySamples;

    while (position < 0.0f)
        position += static_cast<float>(size);

    while (position >= static_cast<float>(size))
        position -= static_cast<float>(size);

    const int indexA =
        static_cast<int>(position);

    const int indexB =
        (indexA + 1) % size;

    const float fraction =
        position -
        static_cast<float>(indexA);

    const float a =
        buffer.getSample(
            channel,
            indexA);

    const float b =
        buffer.getSample(
            channel,
            indexB);

    return a +
           fraction * (b - a);
}

void RGBlueDelayAudioProcessor::DelayLine::advance()
{
    ++writePosition;

    if (writePosition >= size)
        writePosition = 0;
}

//==============================================================

void RGBlueDelayAudioProcessor::ChannelState::reset()
{
    inputHP.reset();
    inputLP.reset();

    delayHP.reset();
    delayLP.reset();

    feedbackHP.reset();
    feedbackLP.reset();

    outputLP.reset();

    feedbackState = 0.0f;
    delayClock = 0.0f;
}

//==============================================================

RGBlueDelayAudioProcessor::RGBlueDelayAudioProcessor()
    : AudioProcessor(
        BusesProperties()
        .withInput(
            "Input",
            juce::AudioChannelSet::stereo(),
            true)
        .withOutput(
            "Output",
            juce::AudioChannelSet::stereo(),
            true)),
      parameters(
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
}

RGBlueDelayAudioProcessor::~RGBlueDelayAudioProcessor()
{
}

//==============================================================

juce::AudioProcessorValueTreeState::ParameterLayout
RGBlueDelayAudioProcessor::createParameterLayout()
{
    std::vector<
        std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "DELAY",
            "Delay",
            juce::NormalisableRange<float>(
                minDelayMs,
                maxDelayMs,
                0.1f),
            300.0f));

    p.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "REPEAT",
            "Repeat",
            juce::NormalisableRange<float>(
                0.0f,
                maxFeedback,
                0.001f),
            0.45f));

    p.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "MIX",
            "Mix",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f,
                0.001f),
            0.32f));

    p.push_back(
        std::make_unique<juce::AudioParameterBool>(
            "BYPASS",
            "Bypass",
            false));

    return {
        p.begin(),
        p.end()
    };
}

//==============================================================

void RGBlueDelayAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    currentSampleRate = sampleRate;

    delayBufferSize =
        static_cast<int>(
            std::ceil(
                sampleRate *
                (maxDelayMs / 1000.0))) + 8;

    delayBuffer.setSize(
        getTotalNumOutputChannels(),
        delayBufferSize);

    delayBuffer.clear();

    writePosition = 0;

    delaySmoothed.reset(
        sampleRate,
        0.025);

    feedbackSmoothed.reset(
        sampleRate,
        0.025);

    mixSmoothed.reset(
        sampleRate,
        0.025);

    bypassSmoothed.reset(
        sampleRate,
        0.010);

    delaySmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue(
                "DELAY")
            ->load());

    feedbackSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue(
                "REPEAT")
            ->load());

    mixSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue(
                "MIX")
            ->load());

    bypassSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue(
                "BYPASS")
            ->load());

    for (auto& state : channelState)
        state.reset();

    updateParameters();
}

//==============================================================

void RGBlueDelayAudioProcessor::releaseResources()
{
    delayBuffer.setSize(0, 0);
}

//==============================================================

bool RGBlueDelayAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()
               == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet()
               == juce::AudioChannelSet::stereo();
}

//==============================================================

void RGBlueDelayAudioProcessor::updateParameters()
{
    for (auto& state : channelState)
    {
        state.inputHP.setHighPass(
            currentSampleRate,
            inputHighPassHz);

        state.inputLP.setLowPass(
            currentSampleRate,
            inputLowPassHz);

        state.delayHP.setHighPass(
            currentSampleRate,
            delayHighPassHz);

        state.delayLP.setLowPass(
            currentSampleRate,
            delayLowPassHz);

        state.feedbackHP.setHighPass(
            currentSampleRate,
            feedbackHighPassHz);

        state.feedbackLP.setLowPass(
            currentSampleRate,
            feedbackLowPassHz);

        state.outputLP.setLowPass(
            currentSampleRate,
            outputLowPassHz);
    }
}

//==============================================================

void RGBlueDelayAudioProcessor::resetDSP()
{
    delayBuffer.clear();

    writePosition = 0;

    for (auto& state : channelState)
        state.reset();
}

//==============================================================

float RGBlueDelayAudioProcessor::readDelaySample(
    int channel,
    float delaySamples) const
{
    if (delayBufferSize <= 1)
        return 0.0f;

    float position =
        static_cast<float>(writePosition)
        - delaySamples;

    while (position < 0.0f)
        position +=
            static_cast<float>(delayBufferSize);

    while (position >=
           static_cast<float>(delayBufferSize))
    {
        position -=
            static_cast<float>(delayBufferSize);
    }

    const int a =
        static_cast<int>(position);

    const int b =
        (a + 1) % delayBufferSize;

    const float fraction =
        position -
        static_cast<float>(a);

    const float sampleA =
        delayBuffer.getSample(
            channel,
            a);

    const float sampleB =
        delayBuffer.getSample(
            channel,
            b);

    return sampleA +
           fraction *
           (sampleB - sampleA);
}

//==============================================================

float RGBlueDelayAudioProcessor::processDelaySample(
    int channel,
    float input,
    float delaySamples,
    float feedback,
    float mix)
{
    auto& state =
        channelState[
            juce::jlimit(0, 1, channel)];

    float dry =
        input;

    float inputSignal =
        state.inputHP.process(dry);

    inputSignal =
        state.inputLP.process(inputSignal);

    const float delayed =
        readDelaySample(
            channel,
            delaySamples);

    float echo =
        state.delayHP.process(
            delayed);

    echo =
        state.delayLP.process(
            echo);

    float feedbackSignal =
        state.feedbackHP.process(
            echo);

    feedbackSignal =
        state.feedbackLP.process(
            feedbackSignal);

    feedbackSignal =
        std::tanh(
            feedbackSignal *
            (1.0f +
             feedbackSoftness *
             feedback));

    const float feedbackGain =
        feedback *
        (R3 /
         (R2 + R3));

    float delayInput =
        inputSignal +
        feedbackSignal *
        feedbackGain;

    delayInput =
        std::tanh(
            delayInput *
            TL072Gain *
            0.12f);

    delayBuffer.setSample(
        channel,
        writePosition,
        delayInput);

    float output =
        dry * (1.0f - mix)
        + echo * mix;

    output =
        state.outputLP.process(
            output);

    return output;
}

//==============================================================

void RGBlueDelayAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    const int numSamples =
        buffer.getNumSamples();

    const int numChannels =
        std::min(
            buffer.getNumChannels(),
            2);

    delaySmoothed.setTargetValue(
        parameters
            .getRawParameterValue(
                "DELAY")
            ->load());

    feedbackSmoothed.setTargetValue(
        parameters
            .getRawParameterValue(
                "REPEAT")
            ->load());

    mixSmoothed.setTargetValue(
        parameters
            .getRawParameterValue(
                "MIX")
            ->load());

    bypassSmoothed.setTargetValue(
        parameters
            .getRawParameterValue(
                "BYPASS")
            ->load());

    for (int s = 0;
         s < numSamples;
         ++s)
    {
        const float delayMs =
            delaySmoothed.getNextValue();

        const float feedback =
            feedbackSmoothed.getNextValue();

        const float mix =
            mixSmoothed.getNextValue();

        const float bypass =
            bypassSmoothed.getNextValue();

        const float delaySamples =
            delayMs *
            static_cast<float>(
                currentSampleRate /
                1000.0);

        for (int ch = 0;
             ch < numChannels;
             ++ch)
        {
            const float dry =
                buffer.getSample(
                    ch,
                    s);

            const float wet =
                processDelaySample(
                    ch,
                    dry,
                    delaySamples,
                    feedback,
                    mix);

            const float finalSample =
                wet * (1.0f - bypass)
                + dry * bypass;

            buffer.setSample(
                ch,
                s,
                finalSample);
        }

        ++writePosition;

        if (writePosition >= delayBufferSize)
            writePosition = 0;
    }
}

//==============================================================

bool RGBlueDelayAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
RGBlueDelayAudioProcessor::createEditor()
{
    return new RGBlueDelayAudioProcessorEditor(*this);
}

//==============================================================

const juce::String
RGBlueDelayAudioProcessor::getName() const
{
    return "RG Blue Delay";
}

bool RGBlueDelayAudioProcessor::acceptsMidi() const
{
    return false;
}

bool RGBlueDelayAudioProcessor::producesMidi() const
{
    return false;
}

bool RGBlueDelayAudioProcessor::isMidiEffect() const
{
    return false;
}

double RGBlueDelayAudioProcessor::getTailLengthSeconds() const
{
    return maxDelayMs / 1000.0;
}

//==============================================================

int RGBlueDelayAudioProcessor::getNumPrograms()
{
    return 1;
}

int RGBlueDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RGBlueDelayAudioProcessor::setCurrentProgram(
    int index)
{
    juce::ignoreUnused(index);
}

const juce::String
RGBlueDelayAudioProcessor::getProgramName(
    int index)
{
    juce::ignoreUnused(index);
    return {};
}

void RGBlueDelayAudioProcessor::changeProgramName(
    int index,
    const juce::String& name)
{
    juce::ignoreUnused(
        index,
        name);
}

//==============================================================

void RGBlueDelayAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state =
        parameters.copyState();

    std::unique_ptr<juce::XmlElement> xml =
        state.createXml();

    copyXmlToBinary(
        *xml,
        destData);
}

//==============================================================

void RGBlueDelayAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml =
        getXmlFromBinary(
            data,
            sizeInBytes);

    if (xml != nullptr &&
        xml->hasTagName(
            parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(
                *xml));
    }
}

//==============================================================

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new RGBlueDelayAudioProcessor();
}
