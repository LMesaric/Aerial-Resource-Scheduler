#pragma once

#include "Schedule.h"

#include <limits>


namespace objective {
    inline double combineObjectives(double aNegativeSum, double aMinSurplus, double aTotalWaterOutput,
                                    const ObjectiveFunctionMultipliers &aMultipliers) {
        return aMultipliers.theA1 * aNegativeSum
               + aMultipliers.theA2 * aMinSurplus
               + aMultipliers.theA3 * aTotalWaterOutput;
    }

    struct Components {
        double theNegativeSum{};
        double theMinSurplus{};
        double theTotalWaterOutput{};
        double theObjective{};

        Components() = default;

        explicit Components(const Schedule &aSchedule) :
                theNegativeSum(aSchedule.getNegativeSurplusSum()),
                theMinSurplus(aSchedule.getMinimumSurplus()),
                theTotalWaterOutput(aSchedule.getTotalWaterOutput()),
                theObjective(combineObjectives(theNegativeSum, theMinSurplus, theTotalWaterOutput,
                                               aSchedule.getInstance().theObjectiveFunctionMultipliers)) {}
    };

    double evaluateObjective(const Schedule &aSchedule) {
        // Not using methods from Schedule for increased performance.

        double myNegativeSum = 0.0;
        double myMinSurplus = std::numeric_limits<double>::max();

        for (const double mySurplus: aSchedule.getWaterTargetSurplus().getRawData()) {
            myNegativeSum += std::min(0.0, mySurplus);
            myMinSurplus = std::min(myMinSurplus, mySurplus);
        }

        return combineObjectives(
                myNegativeSum,
                myMinSurplus,
                aSchedule.getTotalWaterOutput(),
                aSchedule.getInstance().theObjectiveFunctionMultipliers);
    }
}
