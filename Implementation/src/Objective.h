#pragma once

#include "Matrix.h"
#include "Schedule.h"

#include <cfloat>


double evaluateObjective(const Schedule &aSchedule) {
    // Not using methods from Schedule for increased performance.

    double myNegativeSum = 0.0;
    double myMinSurplus = +DBL_MAX;

    for (const double mySurplus: aSchedule.getWaterTargetSurplus().getRawData()) {
        myNegativeSum += std::min(0.0, mySurplus);
        myMinSurplus = std::min(myMinSurplus, mySurplus);
    }

    const auto &myMultipliers = aSchedule.getInstance().theObjectiveFunctionMultipliers;

    return myMultipliers.theA1 * myNegativeSum
           + myMultipliers.theA2 * myMinSurplus
           + myMultipliers.theA3 * aSchedule.getTotalWaterOutput();
}
