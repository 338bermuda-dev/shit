cmake_minimum_required(VERSION 3.22)
project(TrapSnare VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")

if(DEFINED JUCE_SOURCE_DIR)
    add_subdirectory(${JUCE_SOURCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(TrapSnare
    COMPANY_NAME             "Trap Snare"
    BUNDLE_ID                com.trapsnare.plugin
    PRODUCT_NAME             "Trap Snare"
    PLUGIN_MANUFACTURER_CODE Trsn
    PLUGIN_CODE              Tsn1
    FORMATS                  VST3 AU
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES          Instrument Drum
    AU_MAIN_TYPE             kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD  FALSE)

target_sources(TrapSnare PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)

target_compile_definitions(TrapSnare PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

option(TS_MINIMAL_X11 "Build without Xrandr/Xinerama/Xcursor on Linux" OFF)
if(TS_MINIMAL_X11)
    target_compile_definitions(TrapSnare PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_link_libraries(TrapSnare
    PRIVATE juce::juce_audio_processors juce::juce_gui_basics
    PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

option(TS_BUILD_TESTS "Build offline render test + UI snapshot" OFF)
if(TS_BUILD_TESTS)
    juce_add_console_app(RenderTest PRODUCT_NAME "RenderTest")
    target_sources(RenderTest PRIVATE tests/RenderTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(RenderTest PRIVATE Source)
    target_compile_definitions(RenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Trap Snare"
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
    target_link_libraries(RenderTest PRIVATE juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
