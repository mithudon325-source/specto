cmake_minimum_required(VERSION 3.22)
# Static runtime: the plugin runs on any Windows PC without installing Visual C++ redistributables
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(OctaveSpectrogram VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
  GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OctaveSpectrogram
  COMPANY_NAME "OctaveTools"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  PLUGIN_MANUFACTURER_CODE Octa
  PLUGIN_CODE Ospc
  FORMATS VST3 Standalone
  PRODUCT_NAME "Octave Spectrogram")

target_sources(OctaveSpectrogram PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(OctaveSpectrogram PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(OctaveSpectrogram
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
