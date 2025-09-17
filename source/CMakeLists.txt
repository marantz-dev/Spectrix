cmake_minimum_required(VERSION 3.22)

project(midiSnare VERSION 0.0.1)

juce_add_plugin(
  midiSnare
  COMPANY_NAME user 
  PLUGIN_MANUFACTURER_CODE Juce
  PLUGIN_CODE Dem0
  FORMATS VST3 Standalone
  PRODUCT_NAME "midiSnare"
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

juce_generate_juce_header(midiSnare)

target_sources(midiSnare PRIVATE PluginEditor.cpp PluginProcessor.cpp)

target_compile_definitions(
  midiSnare
  PUBLIC
         JUCE_WEB_BROWSER=0
         JUCE_USE_CURL=0
         JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(
  midiSnare
  PRIVATE
          juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
