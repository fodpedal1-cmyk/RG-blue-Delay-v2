#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cmath>

namespace
{
class RealisticLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(
        juce::Graphics&,
        juce::Button&,
        const juce::Colour&,
        bool,
        bool) override
    {
        // Transparent.
        // Footswitch is painted manually in the editor.
    }

    void drawButtonText(
        juce::Graphics&,
        juce::TextButton&,
        bool,
        bool) override
    {
        // No text or rectangle.
    }

    void drawRotarySlider(
        juce::Graphics& g,
        int x,
        int y,
        int w,
        int h,
        float value,
        float startAngle,
        float endAngle,
        juce::Slider&) override
    {
        auto r =
            juce::Rectangle<float>(
                (float)x,
                (float)y,
                (float)w,
                (float)h);

        auto c = r.getCentre();

        float radius =
            juce::jmin(
                r.getWidth(),
                r.getHeight()) * 0.39f;

        //==========================================================
        // SHADOW
        //==========================================================

        g.setColour(
            juce::Colours::black.withAlpha(0.65f));

        g.fillEllipse(
            c.x - radius + 4.0f,
            c.y - radius + 6.0f,
            radius * 2.0f,
            radius * 2.0f);

        //==========================================================
        // OUTER METAL RING
        //==========================================================

        g.setColour(
            juce::Colour(22, 24, 26));

        g.fillEllipse(
            c.x - radius,
            c.y - radius,
            radius * 2.0f,
            radius * 2.0f);

        g.setColour(
            juce::Colour(135, 139, 142));

        g.drawEllipse(
            c.x - radius,
            c.y - radius,
            radius * 2.0f,
            radius * 2.0f,
            2.0f);

        //==========================================================
        // METAL KNOB
        //==========================================================

        float kr = radius * 0.83f;

        juce::ColourGradient metal(
            juce::Colour(235, 237, 238),
            c.x,
            c.y - kr,

            juce::Colour(52, 55, 58),
            c.x,
            c.y + kr,

            false);

        g.setGradientFill(metal);

        g.fillEllipse(
            c.x - kr,
            c.y - kr,
            kr * 2.0f,
            kr * 2.0f);

        g.setColour(
            juce::Colour(205, 208, 210));

        g.drawEllipse(
            c.x - kr,
            c.y - kr,
            kr * 2.0f,
            kr * 2.0f,
            1.5f);

        //==========================================================
        // KNOB GRIP
        //==========================================================

        g.setColour(
            juce::Colours::black.withAlpha(0.35f));

        for (int i = 0; i < 24; ++i)
        {
            float a =
                juce::MathConstants<float>::twoPi
                * (float)i / 24.0f;

            float x1 =
                c.x + std::cos(a) * kr * 0.88f;

            float y1 =
                c.y + std::sin(a) * kr * 0.88f;

            float x2 =
                c.x + std::cos(a) * kr * 0.97f;

            float y2 =
                c.y + std::sin(a) * kr * 0.97f;

            g.drawLine(
                x1,
                y1,
                x2,
                y2,
                1.0f);
        }

        //==========================================================
        // POSITION MARKER
        //==========================================================

        float angle =
            startAngle +
            value * (endAngle - startAngle);

        float marker =
            kr * 0.68f;

        float mx =
            c.x + std::cos(angle) * marker;

        float my =
            c.y + std::sin(angle) * marker;

        g.setColour(
            juce::Colours::white);

        g.drawLine(
            c.x,
            c.y,
            mx,
            my,
            3.5f);

        //==========================================================
        // CENTRE CAP
        //==========================================================

        g.setColour(
            juce::Colour(30, 32, 34));

        g.fillEllipse(
            c.x - 7.0f,
            c.y - 7.0f,
            14.0f,
            14.0f);
    }
};

RealisticLookAndFeel pedalLookAndFeel;
}

//================================================================
// CONSTRUCTOR
//================================================================

RGBlueDelayAudioProcessorEditor::
RGBlueDelayAudioProcessorEditor(
    RGBlueDelayAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
{
    setupKnob(
        delaySlider,
        delayLabel,
        "DELAY");

    setupKnob(
        repeatSlider,
        repeatLabel,
        "REPEAT");

    setupKnob(
        mixSlider,
        mixLabel,
        "MIX");

    //==============================================================
    // PARAMETER ATTACHMENTS
    //==============================================================

    delayAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "DELAY",
                delaySlider);

    repeatAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "REPEAT",
                repeatSlider);

    mixAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "MIX",
                mixSlider);

    //==============================================================
    // FOOTSWITCH
    //==============================================================

    footswitchButton.setClickingTogglesState(true);

    footswitchButton.setWantsKeyboardFocus(false);

    footswitchButton.setMouseClickGrabsKeyboardFocus(false);

    footswitchButton.setColour(
        juce::TextButton::buttonColourId,
        juce::Colours::transparentBlack);

    footswitchButton.setColour(
        juce::TextButton::buttonOnColourId,
        juce::Colours::transparentBlack);

    footswitchButton.setColour(
        juce::TextButton::textColourOffId,
        juce::Colours::transparentBlack);

    footswitchButton.setColour(
        juce::TextButton::textColourOnId,
        juce::Colours::transparentBlack);

    addAndMakeVisible(
        footswitchButton);

    bypassAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.parameters,
                "BYPASS",
                footswitchButton);

    setLookAndFeel(
        &pedalLookAndFeel);

    setSize(
        500,
        640);

    startTimerHz(30);
}

//================================================================
// DESTRUCTOR
//================================================================

RGBlueDelayAudioProcessorEditor::
~RGBlueDelayAudioProcessorEditor()
{
    stopTimer();

    setLookAndFeel(nullptr);
}

//================================================================
// SETUP KNOB
//================================================================

void RGBlueDelayAudioProcessorEditor::setupKnob(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& text)
{
    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::NoTextBox,
        false,
        0,
        0);

    slider.setRange(
        0.0,
        1.0,
        0.001);

    slider.setRotaryParameters(
        2.0f / 3.0f *
            juce::MathConstants<float>::pi,

        7.0f / 3.0f *
            juce::MathConstants<float>::pi,

        true);

    slider.setLookAndFeel(
        &pedalLookAndFeel);

    addAndMakeVisible(
        slider);

    label.setText(
        text,
        juce::dontSendNotification);

    label.setJustificationType(
        juce::Justification::centred);

    label.setColour(
        juce::Label::textColourId,
        juce::Colours::white);

    label.setFont(
        juce::Font(
            juce::FontOptions()
                .withHeight(15.0f)
                .withStyle("bold")));

    addAndMakeVisible(
        label);
}

//================================================================
// TIMER
//================================================================

void RGBlueDelayAudioProcessorEditor::timerCallback()
{
    repaint();
}

//================================================================
// PAINT
//================================================================

void RGBlueDelayAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    auto b =
        getLocalBounds().toFloat();

    //==============================================================
    // 125B METAL ENCLOSURE
    //==============================================================

    juce::ColourGradient body(
        juce::Colour(32, 44, 55),
        0.0f,
        0.0f,

        juce::Colour(8, 13, 18),
        500.0f,
        640.0f,

        false);

    g.setGradientFill(body);

    g.fillRoundedRectangle(
        b.reduced(7.0f),
        18.0f);

    //==============================================================
    // METALLIC RIM
    //==============================================================

    g.setColour(
        juce::Colour(125, 130, 134));

    g.drawRoundedRectangle(
        b.reduced(7.0f),
        18.0f,
        2.0f);

    //==============================================================
    // INNER RIM
    //==============================================================

    g.setColour(
        juce::Colour(12, 16, 20));

    g.drawRoundedRectangle(
        b.reduced(13.0f),
        14.0f,
        1.0f);

    //==============================================================
    // CORNER SCREWS
    //==============================================================

    const float sx[] =
    {
        27.0f,
        473.0f,
        27.0f,
        473.0f
    };

    const float sy[] =
    {
        27.0f,
        27.0f,
        613.0f,
        613.0f
    };

    for (int i = 0; i < 4; ++i)
    {
        g.setColour(
            juce::Colour(165, 168, 170));

        g.fillEllipse(
            sx[i] - 5.0f,
            sy[i] - 5.0f,
            10.0f,
            10.0f);

        g.setColour(
            juce::Colour(55, 58, 60));

        g.drawLine(
            sx[i] - 3.0f,
            sy[i] - 3.0f,
            sx[i] + 3.0f,
            sy[i] + 3.0f,
            1.2f);
    }

    //==============================================================
    // DIVIDER
    //==============================================================

    g.setColour(
        juce::Colour(100, 110, 118));

    g.drawLine(
        45.0f,
        382.0f,
        455.0f,
        382.0f,
        1.0f);

    //==============================================================
    // TITLE
    //==============================================================

    g.setColour(
        juce::Colours::white);

    g.setFont(
        juce::Font(
            juce::FontOptions()
                .withHeight(27.0f)
                .withStyle("bold")));

    g.drawFittedText(
        "RG BLUE DELAY",
        juce::Rectangle<int>(
            50,
            392,
            400,
            32),
        juce::Justification::centred,
        1);

    //==============================================================
    // SUBTITLE
    //==============================================================

    g.setColour(
        juce::Colour(170, 190, 205));

    g.setFont(
        juce::Font(
            juce::FontOptions()
                .withHeight(11.0f)));

    g.drawFittedText(
        "ANALOG STYLE DELAY",
        juce::Rectangle<int>(
            50,
            424,
            400,
            20),
        juce::Justification::centred,
        1);

    //==============================================================
    // LED
    //==============================================================

    const float ledCX = 145.0f;
    const float ledCY = 530.0f;

    bool bypassed =
        audioProcessor.parameters
            .getRawParameterValue("BYPASS")
            ->load() > 0.5f;

    if (!bypassed)
    {
        // Glow

        g.setColour(
            juce::Colours::deepskyblue
                .withAlpha(0.20f));

        g.fillEllipse(
            ledCX - 27.0f,
            ledCY - 27.0f,
            54.0f,
            54.0f);

        // LED body

        g.setColour(
            juce::Colours::deepskyblue);

        g.fillEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f);

        // LED highlight

        g.setColour(
            juce::Colours::white
                .withAlpha(0.65f));

        g.fillEllipse(
            ledCX - 7.0f,
            ledCY - 8.0f,
            7.0f,
            7.0f);
    }
    else
    {
        // LED OFF

        g.setColour(
            juce::Colour(30, 32, 34));

        g.fillEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f);

        g.setColour(
            juce::Colour(70, 72, 74));

        g.drawEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f,
            1.0f);
    }

    //==============================================================
    // REALISTIC 3PDT FOOTSWITCH
    // SAME OUTER DIAMETER AS KNOB
    //==============================================================

    const float switchCX = 250.0f;
    const float switchCY = 530.0f;

    const float switchRadius = 64.35f;

    //==============================================================
    // DEEP SHADOW
    //==============================================================

    g.setColour(
        juce::Colours::black
            .withAlpha(0.80f));

    g.fillEllipse(
        switchCX - switchRadius + 5.0f,
        switchCY - switchRadius + 7.0f,
        switchRadius * 2.0f,
        switchRadius * 2.0f);

    //==============================================================
    // OUTER METAL WASHER
    //==============================================================

    juce::ColourGradient washerMetal(
        juce::Colour(230, 232, 233),
        switchCX,
        switchCY - switchRadius,

        juce::Colour(48, 51, 53),
        switchCX,
        switchCY + switchRadius,

        false);

    g.setGradientFill(
        washerMetal);

    g.fillEllipse(
        switchCX - switchRadius,
        switchCY - switchRadius,
        switchRadius * 2.0f,
        switchRadius * 2.0f);

    g.setColour(
        juce::Colour(18, 20, 22));

    g.drawEllipse(
        switchCX - switchRadius,
        switchCY - switchRadius,
        switchRadius * 2.0f,
        switchRadius * 2.0f,
        2.5f);

    //==============================================================
    // INNER BLACK MOUNTING RING
    //==============================================================

    const float innerRadius =
        switchRadius * (35.0f / 43.0f);

    g.setColour(
        juce::Colour(18, 20, 22));

    g.fillEllipse(
        switchCX - innerRadius,
        switchCY - innerRadius,
        innerRadius * 2.0f,
        innerRadius * 2.0f);

    g.setColour(
        juce::Colour(100, 103, 105));

    g.drawEllipse(
        switchCX - innerRadius,
        switchCY - innerRadius,
        innerRadius * 2.0f,
        innerRadius * 2.0f,
        1.5f);

    //==============================================================
    // ROUND METAL SWITCH CAP
    //==============================================================

    const float capRadius =
        switchRadius * (29.0f / 43.0f);

    juce::ColourGradient capMetal(
        juce::Colour(248, 249, 249),
        switchCX - capRadius * 0.40f,
        switchCY - capRadius,

        juce::Colour(62, 65, 67),
        switchCX + capRadius * 0.48f,
        switchCY + capRadius,

        true);

    g.setGradientFill(
        capMetal);

    g.fillEllipse(
        switchCX - capRadius,
        switchCY - capRadius,
        capRadius * 2.0f,
        capRadius * 2.0f);

    // Cap border

    g.setColour(
        juce::Colour(25, 27, 29));

    g.drawEllipse(
        switchCX - capRadius,
        switchCY - capRadius,
        capRadius * 2.0f,
        capRadius * 2.0f,
        2.0f);

    //==============================================================
    // TOP METAL HIGHLIGHT
    //==============================================================

    juce::Path highlightArc;

    highlightArc.addArc(
        switchCX - capRadius * 0.76f,
        switchCY - capRadius * 0.76f,
        capRadius * 1.52f,
        capRadius * 1.52f,
        3.7f,
        5.6f,
        true);

    g.setColour(
        juce::Colours::white
            .withAlpha(0.65f));

    g.strokePath(
        highlightArc,
        juce::PathStrokeType(
            switchRadius * 0.039f));

    //==============================================================
    // LOWER REFLECTION
    //==============================================================

    juce::Path lowerReflection;

    lowerReflection.addArc(
        switchCX - capRadius * 0.79f,
        switchCY - capRadius * 0.79f,
        capRadius * 1.58f,
        capRadius * 1.58f,
        0.3f,
        2.4f,
        true);

    g.setColour(
        juce::Colours::white
            .withAlpha(0.12f));

    g.strokePath(
        lowerReflection,
        juce::PathStrokeType(
            switchRadius * 0.031f));

    //==============================================================
    // CENTRE CONTACT
    //==============================================================

    const float contactRadius =
        switchRadius * (5.0f / 43.0f);

    g.setColour(
        juce::Colour(25, 27, 29));

    g.fillEllipse(
        switchCX - contactRadius,
        switchCY - contactRadius,
        contactRadius * 2.0f,
        contactRadius * 2.0f);

    g.setColour(
        juce::Colour(175, 178, 180));

    const float contactHighlight =
        switchRadius * (2.5f / 43.0f);

    g.fillEllipse(
        switchCX - contactHighlight,
        switchCY - contactHighlight,
        contactHighlight * 2.0f,
        contactHighlight * 2.0f);

    //==============================================================
    // FOOTER
    //==============================================================

    g.setColour(
        juce::Colour(130, 140, 148));

    g.setFont(
        juce::Font(
            juce::FontOptions()
                .withHeight(10.0f)));

    g.drawFittedText(
        "RG ELECTRONICS",
        juce::Rectangle<int>(
            50,
            605,
            400,
            18),
        juce::Justification::centred,
        1);
}

//================================================================
// RESIZED
//================================================================

void RGBlueDelayAudioProcessorEditor::resized()
{
    //==============================================================
    // MIX - UPPER CENTER
    //==============================================================

    mixSlider.setBounds(
        160,
        35,
        180,
        165);

    mixLabel.setBounds(
        185,
        185,
        130,
        25);

    //==============================================================
    // DELAY - LEFT MIDDLE
    //==============================================================

    delaySlider.setBounds(
        35,
        205,
        180,
        165);

    delayLabel.setBounds(
        60,
        350,
        130,
        25);

    //==============================================================
    // REPEAT - RIGHT MIDDLE
    //==============================================================

    repeatSlider.setBounds(
        285,
        205,
        180,
        165);

    repeatLabel.setBounds(
        310,
        350,
        130,
        25);

    //==============================================================
    // FOOTSWITCH
    //==============================================================

    const float switchRadius = 64.35f;

    const int switchX =
        (int) std::round(
            250.0f - switchRadius);

    const int switchY =
        (int) std::round(
            530.0f - switchRadius);

    const int switchSize =
        (int) std::round(
            switchRadius * 2.0f);

    // Clickable area exactly follows
    // the visual footswitch.

    footswitchButton.setBounds(
        switchX,
        switchY,
        switchSize,
        switchSize);
}
