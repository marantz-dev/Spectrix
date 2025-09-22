# Spectrix – Spectral Compressor

**Spectrix** is an experimental spectral dynamics processor built with [JUCE](https://juce.com/).  
It applies dynamic range compression across the frequency spectrum for more precise control than traditional broadband compressors.

---

## Features
- Multi-band compression in the frequency domain  
- Real-time audio processing with JUCE DSP  
- VST3 and Standalone formats  
- Cross-platform (macOS & Windows)  

---

## Requirements
- [JUCE 8](https://github.com/juce-framework/JUCE) installed locally  
- CMake ≥ 3.22  
- C++20 compatible compiler  
- Xcode (macOS) or Visual Studio 2022 (Windows)  

---

## Build Instructions

### macOS (Xcode)
```bash
# Clone the repo
git clone https://github.com/yourname/Spectrix.git
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
git clone https://github.com/yourname/Spectrix.git
cd Spectrix

# Generate Visual Studio solution
cmake -B build/VS2022 -G "Visual Studio 17 2022" -A x64

# Build (Debug)
cmake --build build/VS2022 --config Debug
```
Open the generated .sln in Builds/VS2022 to run/debug.
