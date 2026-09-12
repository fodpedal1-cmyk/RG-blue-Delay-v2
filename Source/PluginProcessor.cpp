#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

namespace
{
    constexpr float minDelayMs = 25.0f;
    constexpr float maxDelayMs = 450.0f;

    constexpr float maxFeedback = 0.985f;

    constexpr float R1 = 180000.0f;
    constexpr float R2 = 360000.0f;
    constexpr float R3 = 22000.0f;
    constexpr float R4 = 12000.0f;
    constexpr float R5 = 1000.0f;

    constexpr float R6 = 100000.0f;
    constexpr float R7 = 10000.0f;
    constexpr float R8 = 10000.0f;
    constexpr float R9 = 10000.0f;
    constexpr float R10 = 2700.0f;

    constexpr float R11 = 10000.0f;
    constexpr float R12 = 10000.0f;
    constexpr float R13 = 20000.0f;
    constexpr float R14 = 1000.0f;
    constexpr float R15 = 2000.0f;

    constexpr float R16 = 5100.0f;
    constexpr float R17 = 20000.0f;
    constexpr float R18 = 33.0f;
    constexpr float R19 = 10000.0f;
    constexpr float R20 = 10000.0f;

    constexpr float C1 = 22e-9f;
    constexpr float C2 = 47e-12f;
    constexpr float C3 = 100e-12f;
    constexpr float C4 = 1e-6f;
    constexpr float C5 = 1e-6f;
    constexpr float C6 = 4.7e-9f;
    constexpr float C7 = 2.2e-9f;
    constexpr float C8 = 2.2e-9f;

    constexpr float C9 = 100e-9f;
    constexpr float C10 = 100e-9f;
    constexpr float C11 = 100e-9f;
    constexpr float C12 = 100e-9f;

    constexpr float C13 = 15e-9f;
    constexpr float C14 = 2.2e-9f;
    constexpr float C15 = 10e-9f;
    constexpr float C16 = 1e-6f;
    constexpr float C17 = 47e-9f;
    constexpr float C18 = 22e-9f;

    constexpr float C19 = 1e-6f;
    constexpr float C20 = 100e-6f;
    constexpr float C21 = 47e-6f;
    constexpr float C22 = 47e-6f;

    constexpr float inputGain =
        1.0f + (R2 / R1);

    constexpr float inputLowPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R2 *
         C2);

    constexpr float delaySendLowPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R3 *
         C6);

    constexpr float pt2399OutputLowPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R4 *
         C7);

    constexpr float repeatLowPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R15 *
         C17);

    constexpr float repeatHighPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R16 *
         C18);

    constexpr float outputHighPassHz =
        1.0f /
        (2.0f *
         juce::MathConstants<float>::pi *
         R13 *
         C15);

    constexpr float pt2399Smoothing = 0.18f;
}

//==============================================================

void RGBlueDelayAudioProcessor::OnePole::setLowPass(
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
}

void RGBlueDelayAudioProcessor::OnePole::setHighPass(
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

    a = x;
}

float RGBlueDelayAudioProcessor::OnePole::processLowPass(
    float input)
{
    z += a * (input - z);
    return z;
}

float RGBlueDelayAudioProcessor::OnePole::processHighPass(
    float input)
{
    const float low =
        z + a * (input - z);

    const float high =
        input - low;

    z = low;

    return high;
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

    const float sampleA =
        buffer.getSample(
            channel,
            indexA);

    const float sampleB =
        buffer.getSample(
            channel,
            indexB);

    return sampleA +
           fraction * (sampleB - sampleA);
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
    inputLowPass.reset();

    delayLowPass.reset();
    delayHighPass.reset();

    feedbackLowPass.reset();
    feedbackHighPass.reset();

    outputHighPass.reset();

    feedbackMemory = 0.0f;
    pt2399Memory = 0.0f;
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
                1.0f,
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
        2,
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
            .getRawParameterValue("DELAY")
            ->load());

    feedbackSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue("REPEAT")
            ->load());

    mixSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue("MIX")
            ->load());

    bypassSmoothed.setCurrentAndTargetValue(
        parameters
            .getRawParameterValue("BYPASS")
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
        state.inputLowPass.setLowPass(
            currentSampleRate,
            inputLowPassHz);

        state.delayLowPass.setLowPass(
            currentSampleRate,
            delaySendLowPassHz);

        state.delayHighPass.setHighPass(
            currentSampleRate,
            1.0f /
            (2.0f *
             juce::MathConstants<float>::pi *
             R6 *
             C4));

        state.feedbackHighPass.setHighPass(
            currentSampleRate,
            repeatHighPassHz);

        state.feedbackLowPass.setLowPass(
            currentSampleRate,
            repeatLowPassHz);

        state.outputHighPass.setHighPass(
            currentSampleRate,
            outputHighPassHz);
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

float RGBlueDelayAudioProcessor::processPT2399(
    ChannelState& state,
    float input,
    float delayed)
{
    float signal =
        input + delayed;

    signal =
        state.delayLowPass.processLowPass(
            signal);

    state.pt2399Memory +=
        pt2399Smoothing *
        (signal - state.pt2399Memory);

    return state.pt2399Memory;
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

    const float dry =
        input;

    float inputSignal =
        state.inputLowPass.processLowPass(
            dry);

    inputSignal =
        std::tanh(
            inputSignal *
            inputGain *
            0.18f);

    const float delayed =
        readDelaySample(
            channel,
            delaySamples);

    float echo =
        state.delayHighPass.processHighPass(
            delayed);

    echo =
        state.delayLowPass.processLowPass(
            echo);

    float repeat =
        state.feedbackHighPass.processHighPass(
            echo);

    repeat =
        state.feedbackLowPass.processLowPass(
            repeat);

    const float feedbackAmount =
        feedback *
        maxFeedback;

    float feedbackSignal =
        repeat *
        feedbackAmount;

    feedbackSignal =
        std::tanh(
            feedbackSignal *
            1.08f);

    state.feedbackMemory =
        state.feedbackMemory +
        0.12f *
        (feedbackSignal -
         state.feedbackMemory);

    float delayInput =
        inputSignal +
        state.feedbackMemory;

    delayInput =
        std::tanh(
            delayInput *
            1.35f);

    delayBuffer.setSample(
        channel,
        writePosition,
        delayInput);

    float output =
        dry * (1.0f - mix)
        + echo * mix;

    output =
        state.outputHighPass.processHighPass(
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
            .getRawParameterValue("DELAY")
            ->load());

    feedbackSmoothed.setTargetValue(
        parameters
            .getRawParameterValue("REPEAT")
            ->load());

    mixSmoothed.setTargetValue(
        parameters
            .getRawParameterValue("MIX")
            ->load());

    bypassSmoothed.setTargetValue(
        parameters
            .getRawParameterValue("BYPASS")
            ->load());

    for (int sample = 0;
         sample < numSamples;
         ++sample)
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

        for (int channel = 0;
             channel < numChannels;
             ++channel)
        {
            const float dry =
                buffer.getSample(
                    channel,
                    sample);

            const float processed =
                processDelaySample(
                    channel,
                    dry,
                    delaySamples,
                    feedback,
                    mix);

            const float output =
                processed *
                (1.0f - bypass)
                +
                dry *
                bypass;

            buffer.setSample(
                channel,
                sample,
                output);
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
