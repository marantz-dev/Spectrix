
#include "PluginParameters.h"
#include "juce_graphics/juce_graphics.h"
#include <JuceHeader.h>
#include <array>

class GaussianResponseCurve {
  public:
    void updateState() {}

  private:
    struct Peak {
        float frequency;
        float gain;
        float sigmaNorm;
    };

    juce::Path curvePath;
    std::array<float, Parameters::FFT_SIZE> curveSampled;
    std::vector<Peak> peaks;
};
