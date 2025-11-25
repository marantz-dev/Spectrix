cmake_minimum_required(VERSION 3.22)
project(Spectrix VERSION 0.0.1)
set(JUCE_DISABLE_ADHOC_CODE_SIGNING ON)

juce_add_plugin(
  Spectrix
  COMPANY_NAME user
  PLUGIN_MANUFACTURER_CODE Juce
  PLUGIN_CODE Dem0
  FORMATS VST3 Standalone
  PRODUCT_NAME "Spectrix"
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
)

juce_generate_juce_header(Spectrix)

juce_add_binary_data(BinaryData
    SOURCES
        logo.png
)

# Automatically add all source and header files
file(GLOB_RECURSE SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE HEADERS "*.h" "*.hpp")
target_sources(Spectrix PRIVATE ${SOURCES} ${HEADERS})

# Compile definitions
target_compile_definitions(Spectrix
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link JUCE modules
target_link_libraries(Spectrix
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        # juce::juce_recommended_warning_flags
)

# Include directories for headers
target_include_directories(Spectrix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/DSP
    ${CMAKE_CURRENT_SOURCE_DIR}/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/UTILS
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/Components
    ${CMAKE_CURRENT_SOURCE_DIR}/UI/Sections
)
