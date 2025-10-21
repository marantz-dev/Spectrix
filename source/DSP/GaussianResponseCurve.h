#pragma once

#include "PluginParameters.h"
#include <JuceHeader.h>
#include <cstddef>
#include <vector>

struct GaussianPeak {
    float frequency;
    float gainDB;
    float sigmaNorm;
};
class GaussianResponseCurve {
  public:
    GaussianResponseCurve() { addPeak({1000.0f, 0.0f, 0.25f}); }

    void addPeak(GaussianPeak newPeak) {
        const std::lock_guard<std::mutex> lock(mutex);
        gaussians.push_back(newPeak);
    }

    void deletePeak(size_t index) {
        const std::lock_guard<std::mutex> lock(mutex);
        if(index < gaussians.size())
            gaussians.erase(gaussians.begin() + index);
    }

    std::vector<GaussianPeak> &getGaussianPeaks() {
        std::lock_guard<std::mutex> lock(mutex);
        return gaussians;
    }

    void setResponseCurveShiftDB(float newShiftDB) {
        const std::lock_guard<std::mutex> lock(mutex);
        responseCurveShiftDB = newShiftDB;
    }
    float getResponseCurveShiftDB() const { return responseCurveShiftDB; }

  private:
    float responseCurveShiftDB = Parameters::responseCurveShiftDB;
    mutable std::mutex mutex;
    std::vector<GaussianPeak> gaussians;
};
