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

    ValueTree toValueTree() const {
        ValueTree tree("GaussianResponse");

        tree.setProperty("responseCurveShiftDB", responseCurveShiftDB, nullptr);

        ValueTree peaksTree("Peaks");
        for(const auto &peak : gaussians) {
            ValueTree peakNode("Peak");
            peakNode.setProperty("frequency", peak.frequency, nullptr);
            peakNode.setProperty("gainDB", peak.gainDB, nullptr);
            peakNode.setProperty("sigmaNorm", peak.sigmaNorm, nullptr);
            peaksTree.addChild(peakNode, -1, nullptr);
        }
        tree.addChild(peaksTree, -1, nullptr);

        return tree;
    }

    void fromValueTree(const ValueTree &tree) {
        const std::lock_guard<std::mutex> lock(mutex);

        responseCurveShiftDB
         = (float)tree.getProperty("responseCurveShiftDB", Parameters::defaultCurveShiftDB);
        gaussians.clear();

        if(auto peaksTree = tree.getChildWithName("Peaks"); peaksTree.isValid()) {
            for(int i = 0; i < peaksTree.getNumChildren(); ++i) {
                auto peakNode = peaksTree.getChild(i);
                GaussianPeak peak;
                peak.frequency = (float)peakNode.getProperty("frequency", 1000.0f);
                peak.gainDB = (float)peakNode.getProperty("gainDB", 0.0f);
                peak.sigmaNorm = (float)peakNode.getProperty("sigmaNorm", 0.25f);
                gaussians.push_back(peak);
            }
        }
    }

  private:
    float responseCurveShiftDB = Parameters::defaultCurveShiftDB;
    mutable std::mutex mutex;
    std::vector<GaussianPeak> gaussians;
};
