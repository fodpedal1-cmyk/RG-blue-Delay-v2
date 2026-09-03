cmake_minimum_required(VERSION 3.22)

project(RGBlueDelay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RGBlueDelay
    COMPANY_NAME "RG Electronics"
    BUNDLE_ID "com.rgelectronics.rgbluedelay"
    PLUGIN_MANUFACTURER_CODE RGEL
    PLUGIN_CODE BODL

    FORMATS VST3

    PRODUCT_NAME "RG Blue Delay"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(RGBlueDelay)

target_sources(RGBlueDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

target_compile_definitions(RGBlueDelay
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RGBlueDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
)
