#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cmath>

namespace
{
    //==============================================================
    // RG BLUE DELAY LOOK AND FEEL
    //==============================================================

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
            // Footswitch is painted manually.
        }

        void drawButtonText(
            juce::Graphics&,
            juce::TextButton&,
            bool,
            bool) override
        {
            // No button text.
        }

        //==========================================================
        // WHITE PLASTIC KNOB + BLACK POINTER
        //==========================================================

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
            auto r = juce::Rectangle<float>(
                (float) x,
                (float) y,
                (float) w,
                (float) h);

            auto centre = r.getCentre();

            const float radius =
                juce::jmin(
                    r.getWidth(),
                    r.getHeight()) * 0.39f;

            //======================================================
            // SOFT SHADOW
            //======================================================

            g.setColour(
                juce::Colours::black.withAlpha(0.35f));

            g.fillEllipse(
                centre.x - radius + 3.0f,
                centre.y - radius + 5.0f,
                radius * 2.0f,
                radius * 2.0f);

            //======================================================
            // WHITE PLASTIC KNOB
            //======================================================

            const float knobRadius =
                radius * 0.88f;

            juce::ColourGradient plastic(
                juce::Colour(255, 255, 255),
                centre.x - knobRadius * 0.45f,
                centre.y - knobRadius,

                juce::Colour(205, 207, 209),
                centre.x + knobRadius * 0.45f,
                centre.y + knobRadius,

                true);

            g.setGradientFill(plastic);

            g.fillEllipse(
                centre.x - knobRadius,
                centre.y - knobRadius,
                knobRadius * 2.0f,
                knobRadius * 2.0f);

            //======================================================
            // SUBTLE PLASTIC EDGE
            //======================================================

            g.setColour(
                juce::Colour(170, 173, 176));

            g.drawEllipse(
                centre.x - knobRadius,
                centre.y - knobRadius,
                knobRadius * 2.0f,
                knobRadius * 2.0f,
                1.4f);

            //======================================================
            // SUBTLE HIGHLIGHT
            //======================================================

            g.setColour(
                juce::Colours::white.withAlpha(0.55f));

            g.fillEllipse(
                centre.x - knobRadius * 0.52f,
                centre.y - knobRadius * 0.68f,
                knobRadius * 0.65f,
                knobRadius * 0.34f);

            //======================================================
            // BLACK POINTER
            //======================================================

            const float angle =
                startAngle +
                value * (endAngle - startAngle);

            const float pointerStart =
                knobRadius * 0.10f;

            const float pointerEnd =
                knobRadius * 0.70f;

            const float x1 =
                centre.x +
                std::cos(angle) * pointerStart;

            const float y1 =
                centre.y +
                std::sin(angle) * pointerStart;

            const float x2 =
                centre.x +
                std::cos(angle) * pointerEnd;

            const float y2 =
                centre.y +
                std::sin(angle) * pointerEnd;

            g.setColour(
                juce::Colours::black);

            g.drawLine(
                x1,
                y1,
                x2,
                y2,
                4.0f);

            //======================================================
            // SMALL CENTRE CAP
            //======================================================

            g.setColour(
                juce::Colour(245, 245, 245));

            g.fillEllipse(
                centre.x - 5.5f,
                centre.y - 5.5f,
                11.0f,
                11.0f);
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
    //==============================================================
    // KNOBS
    //==============================================================

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

    //==============================================================
    // LOOK AND FEEL
    //==============================================================

    setLookAndFeel(
        &pedalLookAndFeel);

    //==============================================================
    // WINDOW
    //==============================================================

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

    //==============================================================
    // LABEL
    //==============================================================

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
    // LIGHT BLUE METAL ENCLOSURE
    //==============================================================

    juce::ColourGradient body(
        juce::Colour(155, 215, 240),
        0.0f,
        0.0f,

        juce::Colour(65, 145, 190),
        500.0f,
        640.0f,

        false);

    g.setGradientFill(body);

    g.fillRoundedRectangle(
        b.reduced(7.0f),
        18.0f);

    //==============================================================
    // SUBTLE METAL HIGHLIGHT
    //==============================================================

    g.setColour(
        juce::Colours::white.withAlpha(0.10f));

    g.fillRoundedRectangle(
        b.reduced(10.0f).withHeight(150.0f),
        15.0f);

    //==============================================================
    // METALLIC OUTER RIM
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
        juce::Colour(30, 70, 90));

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
            juce::Colour(225, 228, 230));

        g.fillEllipse(
            sx[i] - 5.0f,
            sy[i] - 5.0f,
            10.0f,
            10.0f);

        g.setColour(
            juce::Colour(70, 75, 78));

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
        juce::Colour(225, 245, 255).withAlpha(0.65f));

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
        juce::Colour(225, 245, 255));

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
    // WHITE LED
    //==============================================================

    const float ledCX = 145.0f;
    const float ledCY = 530.0f;

    const bool bypassed =
        audioProcessor.parameters
            .getRawParameterValue("BYPASS")
            ->load() > 0.5f;

    if (!bypassed)
    {
        //==========================================================
        // WHITE GLOW
        //==========================================================

        g.setColour(
            juce::Colours::white.withAlpha(0.22f));

        g.fillEllipse(
            ledCX - 30.0f,
            ledCY - 30.0f,
            60.0f,
            60.0f);

        g.setColour(
            juce::Colours::white.withAlpha(0.12f));

        g.fillEllipse(
            ledCX - 38.0f,
            ledCY - 38.0f,
            76.0f,
            76.0f);

        //==========================================================
        // LED BODY
        //==========================================================

        g.setColour(
            juce::Colours::white);

        g.fillEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f);

        //==========================================================
        // LED HIGHLIGHT
        //==========================================================

        g.setColour(
            juce::Colours::white);

        g.fillEllipse(
            ledCX - 7.0f,
            ledCY - 8.0f,
            7.0f,
            7.0f);
    }
    else
    {
        //==========================================================
        // LED OFF
        //==========================================================

        g.setColour(
            juce::Colour(55, 65, 70));

        g.fillEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f);

        g.setColour(
            juce::Colour(120, 130, 135));

        g.drawEllipse(
            ledCX - 12.0f,
            ledCY - 12.0f,
            24.0f,
            24.0f,
            1.0f);
    }

    //==============================================================
    // REALISTIC 3PDT FOOTSWITCH
    //==============================================================

    const float switchCX = 250.0f;
    const float switchCY = 530.0f;

    const float switchRadius = 64.35f;

    //==============================================================
    // DEEP SHADOW
    //==============================================================

    g.setColour(
        juce::Colours::black.withAlpha(0.80f));

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

    //==============================================================
    // CAP BORDER
    //==============================================================

    g.setColour(
        juce::Colour(25, 27, 29));

    g.drawEllipse(
        switchCX - capRadius,
        switchCY - capRadius,
        capRadius * 2.0f,
        capRadius * 2.0f,
        2.0f);

    //==============================================================
    // TOP HIGHLIGHT
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
        juce::Colours::white.withAlpha(0.65f));

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
        juce::Colours::white.withAlpha(0.12f));

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
        juce::Colour(225, 245, 255));

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
    // DELAY - LEFT
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
    // REPEAT - RIGHT
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
    // FOOTSWITCH CLICK AREA
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

    footswitchButton.setBounds(
        switchX,
        switchY,
        switchSize,
        switchSize);

    // Make sure it receives mouse/touch clicks.
    footswitchButton.toFront(false);
}
