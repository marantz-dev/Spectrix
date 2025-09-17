cmake_minimum_required(VERSION 3.22)

project(FFTProcessor VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include(cmake/cpm.cmake)

cpmaddpackage(
  # NAME
  # JUCE
  # GIT_TAG
  # 8.0.6
  # VERSION
  # 8.0.6
  # GITHUB_REPOSITORY
  # juce-framework/JUCE
  NAME JUCE
  SOURCE_DIR /Applications/JUCE/
)
# Add compiler warning utilities
include(cmake/CompilerWarnings.cmake)
include(cmake/Util.cmake)

# Adds all the targets configured in the "plugin" folder.
add_subdirectory(source)
