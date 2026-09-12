#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

namespace
{
    constexpr float pi = 3.14159265358979323846f;

    constexpr float R15 = 5100.0f;
    constexpr float C13 = 22.0e-9f;

    constexpr float R16 = 2000.0f;
    constexpr float C15 = 47.0e-9f;

    constexpr float repeatHighPass =
        1.0f / (2.0f * pi * R15 * C13);

    constexpr float repeatLowPass =
        1.0f / (2.0f * pi * R16 * C15);

    constexpr float minimumDelayMs = 60.0f;
    constexpr float maximumDelayMs = 634.0f;

    constexpr float maximumFeedback = 0.92f;

    constexpr float inputHighPassHz = 20.0f;
    constexpr float inputLowPassHz = 16000.0f;

    constexpr float outputLowPassHz = 12000.0f;
}

//==============================================================================
// ONE POLE
//==============================================================================

void RGBlueDelayAudioProcessor::OnePole::reset()
{
    z = 0.0f;
}

void RGBlueDelayAudioProcessor::OnePole::setLowPass(
    double sampleRate,
    float cutoff)
{
    highPass = false;

    cutoff = juce::jlimit(
        1.0f,
        static_cast<float>(sampleRate * 0.45),
        cutoff);

    const float x =
        std::exp(
            -2.0f *
            pi *
            cutoff /
            static_cast<float>(sampleRate));

    a = 1.0f - x;
}

void RGBlueDelayAudioProcessor::OnePole::setHighPass(
    double sampleRate,
    float cutoff)
{
    highPass = true;

    cutoff = juce::jlimit(
        1.0f,
        static_cast<float>(sampleRate * 0.45),
        cutoff);

    const float x =
        std::exp(
            -2.0f *
            pi *
            cutoff /
            static_cast<float>(sampleRate));

    a = x;
}

float RGBlueDelayAudioProcessor::OnePole::process(float input)
{
    if (!highPass)
    {
        z += a * (input - z);
        return z;
    }

    const float previous = z;

    z = a * (z + input - previous);

    return z;
}

//==============================================================================
// DELAY LINE
//==============================================================================

void RGBlueDelayAudioProcessor::DelayLine::prepare(
    int channels,
    int samples)
{
    size = std::max(2, samples);

    buffer.setSize(
        std::max(1, channels),
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
        channel < buffer.getNumChannels() &&
        size > 0)
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
    if (size <= 1 ||
        channel < 0 ||
        channel >= buffer.getNumChannels())
    {
        return 0.0f;
    }

    delaySamples =
        juce::jlimit(
            1.0f,
            static_cast<float>(size - 2),
            delaySamples);

    float readPosition =
        static_cast<float>(writePosition) -
        delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float>(size);

    while (readPosition >= static_cast<float>(size))
        readPosition -= static_cast<float>(size);

    const int index1 =
        static_cast<int>(readPosition);

    const int index2 =
        (index1 + 1) % size;

    const float fraction =
        readPosition -
        static_cast<float>(index1);

    const float sample1 =
        buffer.getSample(
            channel,
            index1);

    const float sample2 =
        buffer.getSample(
            channel,
            index2);

    return sample1 +
           (sample2 - sample1) *
           fraction;
}

void RGBlueDelayAudioProcessor::DelayLine::advance()
{
    if (size > 0)
    {
        ++writePosition;

        if (writePosition >= size)
            writePosition = 0;
    }
}

//==============================================================================
// CHANNEL STATE
//==============================================================================

void RGBlueDelayAudioProcessor::ChannelState::reset()
{
    feedbackHP.reset();
    feedbackLP.reset();

    previousInput = 0.0f;
    previousOutput = 0.0f;
}

//==============================================================================
// PARAMETER LAYOUT
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
RGBlueDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "DELAY",
            "DELAY",
            juce::NormalisableRange<float>(
                minimumDelayMs,
                maximumDelayMs,
                0.1f),
            300.0f));

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "REPEAT",
            "REPEAT",
            juce::NormalisableRange<float>(
                0.0f,
                maximumFeedback,
                0.001f),
            0.45f));

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "MIX",
            "MIX",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f,
                0.001f),
            0.32f));

    params.push_back(
        std::make_unique<juce::AudioParameterBool>(
            "BYPASS",
            "BYPASS",
            false));

    return {
        params.begin(),
        params.end()
    };
}

//==============================================================================
// CONSTRUCTOR
//==============================================================================

RGBlueDelayAudioProcessor::RGBlueDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
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
          "RG_BLUE_DELAY_STATE",
          createParameterLayout())
#else
    : parameters(
          *this,
          nullptr,
          "RG_BLUE_DELAY_STATE",
          createParameterLayout())
#endif
{
}

//==============================================================================

RGBlueDelayAudioProcessor::~RGBlueDelayAudioProcessor()
{
}

//==============================================================================
// PREPARE
//==============================================================================

void RGBlueDelayAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate =
        std::max(
            22050.0,
            sampleRate);

    currentBlockSize =
        std::max(
            1,
            samplesPerBlock);

    const int maximumDelaySamples =
        static_cast<int>(
            std::ceil(
                currentSampleRate *
                maximumDelayMs /
                1000.0))
        + 4;

    delayLine.prepare(
        2,
        maximumDelaySamples);

    delaySmoothed.reset(
        currentSampleRate,
        0.025);

    feedbackSmoothed.reset(
        currentSampleRate,
        0.025);

    mixSmoothed.reset(
        currentSampleRate,
        0.025);

    bypassSmoothed.reset(
        currentSampleRate,
        0.010);

    for (auto& state : channelState)
    {
        state.feedbackHP.setHighPass(
            currentSampleRate,
            repeatHighPass);

        state.feedbackLP.setLowPass(
            currentSampleRate,
            repeatLowPass);
    }

    updateParameters();

    resetDSP();
}

//==============================================================================

void RGBlueDelayAudioProcessor::releaseResources()
{
    delayLine.clear();

    resetDSP();
}

//==============================================================================

bool RGBlueDelayAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto input =
        layouts.getChannelSet(
            true,
            0);

    const auto output =
        layouts.getChannelSet(
            false,
            0);

    if (output != input)
        return false;

    if (output != juce::AudioChannelSet::mono() &&
        output != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    return true;
}

//==============================================================================
// RESET
//==============================================================================

void RGBlueDelayAudioProcessor::resetDSP()
{
    delayLine.clear();

    for (auto& state : channelState)
        state.reset();

    for (auto& state : channelState)
    {
        state.feedbackHP.setHighPass(
            currentSampleRate,
            repeatHighPass);

        state.feedbackLP.setLowPass(
            currentSampleRate,
            repeatLowPass);
    }
}

//==============================================================================
// PARAMETERS
//==============================================================================

void RGBlueDelayAudioProcessor::updateParameters()
{
    if (auto* p =
        parameters.getRawParameterValue("DELAY"))
    {
        delayTimeMs =
            juce::jlimit(
                minimumDelayMs,
                maximumDelayMs,
                p->load());
    }

    if (auto* p =
        parameters.getRawParameterValue("REPEAT"))
    {
        feedbackAmount =
            juce::jlimit(
                0.0f,
                maximumFeedback,
                p->load());
    }

    if (auto* p =
        parameters.getRawParameterValue("MIX"))
    {
        mixAmount =
            juce::jlimit(
                0.0f,
                1.0f,
                p->load());
    }

    if (auto* p =
        parameters.getRawParameterValue("BYPASS"))
    {
        bypassed =
            p->load() > 0.5f;
    }
}

//==============================================================================
// READ DELAY
//==============================================================================

float RGBlueDelayAudioProcessor::readDelay(
    int channel,
    float delaySamples) const
{
    return delayLine.read(
        channel,
        delaySamples);
}

//==============================================================================
// PROCESS ONE SAMPLE
//==============================================================================

float RGBlueDelayAudioProcessor::processDelaySample(
    int channel,
    float input,
    float delaySamples,
    float feedback,
    float mix)
{
    auto& state =
        channelState[
            juce::jlimit(
                0,
                1,
                channel)];

    const float delayed =
        readDelay(
            channel,
            delaySamples);

    float repeat =
        delayed;

    repeat =
        state.feedbackHP.process(
            repeat);

    repeat =
        state.feedbackLP.process(
            repeat);

    const float feedbackSignal =
        repeat * feedback;

    const float writeSignal =
        input +
        feedbackSignal;

    delayLine.write(
        channel,
        writeSignal);

    const float output =
        input * (1.0f - mix) +
        delayed * mix;

    state.previousInput = input;
    state.previousOutput = output;

    return output;
}

//==============================================================================
// PROCESS BLOCK
//==============================================================================

void RGBlueDelayAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    juce::ignoreUnused(
        midiMessages);

    const int numChannels =
        buffer.getNumChannels();

    const int numSamples =
        buffer.getNumSamples();

    if (numChannels <= 0 ||
        numSamples <= 0)
    {
        return;
    }

    updateParameters();

    if (bypassed)
        return;

    const int channelsToProcess =
        std::min(
            numChannels,
            2);

    delaySmoothed.setTargetValue(
        delayTimeMs);

    feedbackSmoothed.setTargetValue(
        feedbackAmount);

    mixSmoothed.setTargetValue(
        mixAmount);

    const float delaySamplesPerMs =
        static_cast<float>(
            currentSampleRate /
            1000.0);

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

        const float delaySamples =
            juce::jlimit(
                1.0f,
                static_cast<float>(
                    delayLine.size - 2),
                delayMs *
                delaySamplesPerMs);

        for (int channel = 0;
             channel < channelsToProcess;
             ++channel)
        {
            const float input =
                buffer.getSample(
                    channel,
                    sample);

            const float output =
                processDelaySample(
                    channel,
                    input,
                    delaySamples,
                    feedback,
                    mix);

            buffer.setSample(
                channel,
                sample,
                output);
        }

        if (channelsToProcess == 1)
        {
            delayLine.write(
                1,
                delayLine.buffer.getSample(
                    0,
                    delayLine.writePosition));
        }

        delayLine.advance();
    }
}

//==============================================================================
// EDITOR
//==============================================================================

juce::AudioProcessorEditor*
RGBlueDelayAudioProcessor::createEditor()
{
    return new RGBlueDelayAudioProcessorEditor(
        *this);
}

bool RGBlueDelayAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
// BASIC INFO
//==============================================================================

const juce::String
RGBlueDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
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
    return 2.0;
}

//==============================================================================
// PROGRAMS
//==============================================================================

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
    const juce::String& newName)
{
    juce::ignoreUnused(
        index,
        newName);
}

//==============================================================================
// STATE
//==============================================================================

void RGBlueDelayAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state =
        parameters.copyState();

    if (auto xml =
        state.createXml())
    {
        copyXmlToBinary(
            *xml,
            destData);
    }
}

void RGBlueDelayAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    if (auto xmlState =
        getXmlFromBinary(
            data,
            sizeInBytes))
    {
        if (xmlState->hasTagName(
                parameters.state.getType()))
        {
            parameters.replaceState(
                juce::ValueTree::fromXml(
                    *xmlState));
        }
    }

    updateParameters();
}
