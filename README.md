# Spectrix – Spectral Compressor

**Spectrix** is an experimental spectral dynamics processor built with [JUCE](https://juce.com/).  
It applies dynamic range compression across the frequency spectrum for more precise control than traditional broadband compressors.

## Requirements

- [JUCE 8](https://github.com/juce-framework/JUCE) installed locally
- CMake ≥ 3.22
- C++20 compatible compiler
- Xcode (macOS) or Visual Studio 2022 (Windows)

## Build Instructions

‼️ **If you want to use JUCE that is already installled on you system you need to point the cpm package to your JUCE path**

### In Spectrix/CMakeLists.txt

```bash
cpmaddpackage(

  ###This will download JUCE the first time you build###

  NAME JUCE
  GIT_TAG 8.0.6
  VERSION 8.0.6
  GITHUB_REPOSITORY juce-framework/JUCE

  ###This will point cpm to your installed JUCE path###

  NAME JUCE
  SOURCE_DIR /YOUR/JUCE/PATH
)
```

### macOS (Xcode)

```bash
# Clone the repo
git clone https://github.com/marantz-dev/Spectrix.git
cd Spectrix

# Generate Xcode project
cmake -B build/Xcode -G Xcode

# Build (Debug)
cmake --build build/Xcode --config Debug
```

Open the generated .xcodeproj in build/Xcode to run/debug.

### Windows (Visual Studio 2022)

```bash
# Clone the repo
git clone https://github.com/marantz-dev/Spectrix.git
cd Spectrix

# Generate Visual Studio solution
cmake -B build/VS2022 -G "Visual Studio 17 2022" -A x64

# Build (Debug)
cmake --build build/VS2022 --config Debug
```

Open the generated .sln in Builds/VS2022 to run/debug.

- Add 2nd window + Compensation
- Be carefull su cosa calcoli attack e release (sample rate / hopsize)
-
