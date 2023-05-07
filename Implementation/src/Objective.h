#pragma once

#include "Schedule.h"

#include <limits>


namespace objective {
    inline double combineObjectives(double aNegativeSumWithPriorities, double aMinSurplus, double aTotalWaterOutput,
                                    const ObjectiveFunctionMultipliers &aMultipliers) {
        return aMultipliers.theA1 * aNegativeSumWithPriorities
               + aMultipliers.theA2 * aMinSurplus
               + aMultipliers.theA3 * aTotalWaterOutput;
    }

    struct Components {
        double theNegativeSum{};
        double theNegativeSumWithPriorities{};
        double theMinSurplus{};
        double theTotalWaterOutput{};
        double theObjective{};

        Components() = default;

        explicit Components(const Schedule &aSchedule) :
                theNegativeSum(aSchedule.getNegativeSurplusSum()),
                theNegativeSumWithPriorities(aSchedule.getNegativeSurplusSumWithPriorities()),
                theMinSurplus(aSchedule.getMinimumSurplus()),
                theTotalWaterOutput(aSchedule.getTotalWaterOutput()),
                theObjective(combineObjectives(theNegativeSumWithPriorities, theMinSurplus, theTotalWaterOutput,
                                               aSchedule.getInstance().theObjectiveFunctionMultipliers)) {}
    };

    double evaluateObjective(const Schedule &aSchedule) {
        // Not using methods from Schedule for increased performance.

        double myNegativeSumWithPriorities = 0.0;
        double myMinSurplus = std::numeric_limits<double>::max();

        const auto &myFronts = aSchedule.getInstance().theFronts;
        const auto &mySurplusMatrix = aSchedule.getWaterTargetSurplus();
        const auto &mySurplusRawData = mySurplusMatrix.getRawData();
        std::size_t idx = 0;
        for (std::size_t myFrontId = 0; myFrontId < mySurplusMatrix.getX(); ++myFrontId) {
            for (std::size_t mySlot = 0; mySlot < mySurplusMatrix.getY(); ++mySlot) {
                const double mySurplus = mySurplusRawData[idx++];
                if (mySurplus < 0.0) {
                    myNegativeSumWithPriorities += mySurplus * myFronts[myFrontId].thePriority;
                }
                myMinSurplus = std::min(myMinSurplus, mySurplus);
            }
        }

        return combineObjectives(
                myNegativeSumWithPriorities,
                myMinSurplus,
                aSchedule.getTotalWaterOutput(),
                aSchedule.getInstance().theObjectiveFunctionMultipliers
        );
    }
}
