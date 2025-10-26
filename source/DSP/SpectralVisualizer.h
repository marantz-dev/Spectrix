#pragma once
#include "FFTProcessor.h"
#include "GaussianResponseCurve.h"
#include <JuceHeader.h>
#include <array>
#include <cstddef>

template <size_t FFT_SIZE = 512, size_t NUM_CHANNELS = 2>
class SpectralVisualizer : public FFTProcessor<FFT_SIZE, NUM_CHANNELS> {
  public:
    SpectralVisualizer() : FFTProcessor<FFT_SIZE, NUM_CHANNELS>() {}

  private:
    void processFFTBins(std::array<float, FFT_SIZE * 2> &transformedBuffer) override {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralVisualizer)
};
