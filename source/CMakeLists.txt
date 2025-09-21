cmake_minimum_required(VERSION 3.22)

project(FFTProcessor VERSION 0.0.1)

juce_add_plugin(
  FFTProcessor
  COMPANY_NAME user 
  PLUGIN_MANUFACTURER_CODE Juce
  PLUGIN_CODE Dem0
  FORMATS VST3 Standalone
  PRODUCT_NAME "FFTProcessor"
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

juce_generate_juce_header(FFTProcessor)

target_sources(FFTProcessor PRIVATE PluginEditor.cpp PluginProcessor.cpp)

target_compile_definitions(
  FFTProcessor
  PUBLIC
         JUCE_WEB_BROWSER=0
         JUCE_USE_CURL=0
         JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(
  FFTProcessor
  PRIVATE
          juce::juce_audio_utils
          juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
