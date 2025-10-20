#pragma once

#include "PluginParameters.h"
#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <vector>

struct GaussianPeak {
    float frequency;
    float gainDB;
    float sigmaNorm;
};
class GaussianResponseCurve {
  public:
    GaussianResponseCurve() { addPeak({5000.0f, 0.0f, 0.15f}); }

    void addPeak(GaussianPeak newPeak) {
        const std::lock_guard<std::mutex> lock(mutex);
        gaussians.push_back(newPeak);
    }

    void deletePeak(size_t index) {
        const std::lock_guard<std::mutex> lock(mutex);
        if(index < gaussians.size()) {
            gaussians.erase(gaussians.begin() + index);
        }
    }

    // void updatePeak(int index, GaussianPeak newPeak) {
    //     const std::lock_guard<std::mutex> lock(mutex);
    //     if(index < gaussians.size())
    //         gaussians[index] = newPeak;
    // }

    std::vector<GaussianPeak> &getGaussianPeaks() {
        std::lock_guard<std::mutex> lock(mutex);
        return gaussians;
    }

  private:
    mutable std::mutex mutex;
    std::vector<GaussianPeak> gaussians;
};
