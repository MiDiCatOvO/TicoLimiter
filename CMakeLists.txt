cmake_minimum_required(VERSION 3.22)
project(TicoLimiter VERSION 5.2.1 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE from GitHub
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# AAX SDK path — set via -DJUCE_AAX_SDK_PATH=/path/to/aax-sdk
if(NOT DEFINED JUCE_AAX_SDK_PATH AND DEFINED ENV{JUCE_AAX_SDK_PATH})
    set(JUCE_AAX_SDK_PATH $ENV{JUCE_AAX_SDK_PATH})
endif()

# Define the plugin target
juce_add_plugin(TicoLimiter
    COMPANY_NAME "MiDiCat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE KwAu
    PLUGIN_CODE LmDc
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Tico Limiter"
    VST3_CATEGORIES "Fx" "Dynamics"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    BUNDLE_ID "com.midicat.ticolimiter"
)

# Add source files
target_sources(TicoLimiter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PresetManager.h
        Source/DSP/SoftClipper.h
        Source/DSP/HarmonicSaturator.h
        Source/DSP/Limiter.h
        Source/DSP/TiltEQ.h
        Source/DSP/LUFSMeter.h
        Source/GUI/KawaiiLookAndFeel.h
        Source/GUI/ClipperLED.h
        Source/GUI/RainbowSliderLookAndFeel.h
        Source/GUI/LufsDisplay.h
        Source/GUI/MeterComponent.h
        Source/GUI/SpectrumAnalyzer.h
        Source/GUI/GRHistoryDisplay.h
)

# JUCE modules
target_link_libraries(TicoLimiter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(TicoLimiter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        $<$<CXX_COMPILER_ID:MSVC>:_USE_MATH_DEFINES>
)

# Set deployment target for macOS
if(APPLE)
    set_target_properties(TicoLimiter PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
    )
endif()
