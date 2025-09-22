#pragma once
#include <JuceHeader.h>
#include <array>
#include <cstddef>

// TODO:
// implementare la curva di compressione come una somma di una famiglia di gaussiane
// ogni breakpoint:
// frequency = mu
// gain = amp
// quality = sigma
// la curva di compressione sarà la somma di tutte le gaussiane
// y = sum(amp * exp(-0.5 * ((x - mu) / sigma)^2))
// da visualizzare e modificare su scala logaritmica

struct breakPoint {
      float frequency;
      float gain;
      float quality;
};

class CompressionCurve {
    public:
      CompressionCurve() { curve.fill(0.0f); }
      ~CompressionCurve() = default;

      void computeBreakPointCurve(const float &breakpoint) {
            std::array<float, 22000> bpCurve;
            for(size_t i = 0; i < bpCurve.size(); ++i) {
                  bpCurve.at(i) = 5;
            }
      }

    private:
      std::array<float, 22000> curve;
      std::vector<std::array<float, 22000>> breakpoitCurves;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressionCurve)
};
