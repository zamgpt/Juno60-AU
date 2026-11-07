cmake_minimum_required(VERSION 3.22)
project(Juno60AU VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.10
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Juno60AU
  COMPANY_NAME "Juno60AU"
  BUNDLE_ID "audio.juno60au.instrument"
  PLUGIN_MANUFACTURER_CODE Jn60
  PLUGIN_CODE Jn6A
  FORMATS AU
  PRODUCT_NAME "Juno-60"
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(Juno60AU)
target_sources(Juno60AU PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Juno60AU PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(Juno60AU PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(APPLE)
  set_target_properties(Juno60AU PROPERTIES OSX_ARCHITECTURES "arm64;x86_64" MACOSX_DEPLOYMENT_TARGET "11.0")
endif()
