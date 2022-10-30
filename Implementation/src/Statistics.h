#pragma once

#include <cmath>
#include <numeric>
#include <vector>


namespace statistics {
    double mean(const std::vector<double> &aData) {
        return std::accumulate(aData.begin(), aData.end(), 0.0) / (double) aData.size();
    }

    double stdDev(const std::vector<double> &aData, const double aMean) {
        if (aData.size() == 1) {
            return 0.0;
        }

        const auto myVarianceFunc = [aMean](const double anAcc, const double aVal) {
            return anAcc + (aVal - aMean) * (aVal - aMean);
        };

        const double myVariance =
                std::accumulate(aData.begin(), aData.end(), 0.0, myVarianceFunc) / (double) aData.size();
        return std::sqrt(myVariance);
    }

    std::pair<double, double> meanAndDev(const std::vector<double> &aData) {
        const auto myMean = statistics::mean(aData);
        const auto myStdDev = statistics::stdDev(aData, myMean);
        return {myMean, myStdDev};
    }

    std::pair<double, double> meanAndDevTransform(const std::vector<objective::Components> &aData) {
        std::vector<double> myDoublesData;
        std::transform(aData.cbegin(), aData.cend(), std::back_inserter(myDoublesData),
                       [](const objective::Components &c) { return c.theObjective; });
        return meanAndDev(myDoublesData);
    }
}
