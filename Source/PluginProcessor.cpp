#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float minDelayMs = 60.0f;
    constexpr float maxDelayMs = 634.0f;
    constexpr float maxFeedback = 0.92f;

    constexpr float inputHP = 25.0f;
    constexpr float inputLP = 16000.0f;
    constexpr float feedbackHP = 70.0f;
    constexpr float feedbackLP = 7200.0f;
    constexpr float outputLP = 12000.0f;
}

//==============================================================

RGBlueDelayAudioProcessor::RGBlueDelayAudioProcessor()
    : AudioProcessor(
        BusesProperties()
        .withInput("Input",
                   juce::AudioChannelSet::stereo(), true)
        .withOutput("Output",
                    juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS",
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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DELAY", "Delay",
        juce::NormalisableRange<float>(
            minDelayMs, maxDelayMs, 0.1f),
        300.0f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "REPEAT", "Repeat",
        juce::NormalisableRange<float>(
            0.0f, maxFeedback, 0.001f),
        0.45f));

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MIX", "Mix",
        juce::NormalisableRange<float>(
            0.0f, 1.0f, 0.001f),
        0.32f));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        "BYPASS", "Bypass", false));

    return { p.begin(), p.end() };
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
            std::ceil(sampleRate *
                      (maxDelayMs / 1000.0))) + 4;

    delayBuffer.setSize(
        getTotalNumOutputChannels(),
        delayBufferSize);

    delayBuffer.clear();
    writePosition = 0;

    delaySmoothed.reset(sampleRate, 0.025);
    feedbackSmoothed.reset(sampleRate, 0.025);
    mixSmoothed.reset(sampleRate, 0.025);
    bypassSmoothed.reset(sampleRate, 0.010);

    delaySmoothed.setCurrentAndTargetValue(
        parameters.getRawParameterValue("DELAY")->load());

    feedbackSmoothed.setCurrentAndTargetValue(
        parameters.getRawParameterValue("REPEAT")->load());

    mixSmoothed.setCurrentAndTargetValue(
        parameters.getRawParameterValue("MIX")->load());

    bypassSmoothed.setCurrentAndTargetValue(
        parameters.getRawParameterValue("BYPASS")->load());

    updateFilters();
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

void RGBlueDelayAudioProcessor::updateFilters()
{
    inputHighPass.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeHighPass(
            currentSampleRate, inputHP);

    inputLowPass.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, inputLP);

    feedbackHighPass.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeHighPass(
            currentSampleRate, feedbackHP);

    feedbackLowPass.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, feedbackLP);

    outputLowPass.coefficients =
        juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, outputLP);
}

//==============================================================

float RGBlueDelayAudioProcessor::readDelaySample(
    int channel,
    float delaySamples) const
{
    float position =
        static_cast<float>(writePosition) - delaySamples;

    while (position < 0.0f)
        position += static_cast<float>(delayBufferSize);

    while (position >= static_cast<float>(delayBufferSize))
        position -= static_cast<float>(delayBufferSize);

    const int a = static_cast<int>(position);
    const int b = (a + 1) % delayBufferSize;
    const float f = position - static_cast<float>(a);

    const float sa = delayBuffer.getSample(channel, a);
    const float sb = delayBuffer.getSample(channel, b);

    return sa + f * (sb - sa);
}

//==============================================================

void RGBlueDelayAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    delaySmoothed.setTargetValue(
        parameters.getRawParameterValue("DELAY")->load());

    feedbackSmoothed.setTargetValue(
        parameters.getRawParameterValue("REPEAT")->load());

    mixSmoothed.setTargetValue(
        parameters.getRawParameterValue("MIX")->load());

    bypassSmoothed.setTargetValue(
        parameters.getRawParameterValue("BYPASS")->load());

    for (int s = 0; s < numSamples; ++s)
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
                currentSampleRate / 1000.0);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry =
                buffer.getSample(ch, s);

            float input =
                inputHighPass.processSample(dry);

            input =
                inputLowPass.processSample(input);

            const float delayed =
                readDelaySample(ch, delaySamples);

            float repeat =
                feedbackHighPass.processSample(delayed);

            repeat =
                feedbackLowPass.processSample(repeat);

            repeat =
                std::tanh(repeat * 1.35f);

            delayBuffer.setSample(
                ch,
                writePosition,
                input + repeat * feedback);

            float output =
                dry * (1.0f - mix)
                + delayed * mix;

            output =
                outputLowPass.processSample(output);

            const float finalSample =
                output * (1.0f - bypass)
                + dry * bypass;

            buffer.setSample(ch, s, finalSample);
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

void RGBlueDelayAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String
RGBlueDelayAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void RGBlueDelayAudioProcessor::changeProgramName(
    int index,
    const juce::String& name)
{
    juce::ignoreUnused(index, name);
}

//==============================================================

void RGBlueDelayAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml =
        state.createXml();

    copyXmlToBinary(*xml, destData);
}

//==============================================================

void RGBlueDelayAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml =
        getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr &&
        xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new RGBlueDelayAudioProcessor();
}
