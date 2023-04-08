#pragma once

#include "Objective.h"
#include "Schedule.h"
#include "Takeoff.h"

#include <atomic>
#include <limits>
#include <unordered_set>
#include <vector>


// Schedule will remain effectively unchanged, except possible rounding errors,
// but will not be marked const to avoid making copies.
std::pair<std::vector<Takeoff>, double> recursiveFindOptimalCompletion( // NOLINT(misc-no-recursion)
        Schedule &aSchedule,
        std::unordered_set<Takeoff> &anIgnoreTakeoffs,
        std::atomic_bool &aKillSwitch
) {
    if (aKillSwitch) {
        return {{}, std::numeric_limits<double>::lowest()};
    }

    const auto myTakeoffs = aSchedule.findAllLegalTakeoffs();

    if (myTakeoffs.empty()) {
        return {{}, objective::evaluateObjective(aSchedule)};
    }

    std::vector<Takeoff> myOptimalTakeoffs{};
    double myOptimalObjectiveValue = std::numeric_limits<double>::lowest();

    std::vector<Takeoff> myTakeoffsToRemove{};
    myTakeoffsToRemove.reserve(myTakeoffs.size());

    for (const auto &myTakeoff: myTakeoffs) {
        if (anIgnoreTakeoffs.contains(myTakeoff)) {
            continue;
        }

        aSchedule.insertTakeoff(myTakeoff);

        auto[myOptimalTakeoffsTmp, myObjectiveValueTmp] = recursiveFindOptimalCompletion(
                aSchedule, anIgnoreTakeoffs, aKillSwitch
        );

        if (myObjectiveValueTmp > myOptimalObjectiveValue) {
            myOptimalTakeoffsTmp.push_back(myTakeoff);

            myOptimalTakeoffs = std::move(myOptimalTakeoffsTmp);
            myOptimalObjectiveValue = myObjectiveValueTmp;
        }

        myTakeoffsToRemove.push_back(myTakeoff);
        anIgnoreTakeoffs.insert(myTakeoff);

        aSchedule.removeTakeoff(myTakeoff);
    }

    for (const auto &myTakeoff: myTakeoffsToRemove) {
        anIgnoreTakeoffs.erase(myTakeoff);
    }

    return {std::move(myOptimalTakeoffs), myOptimalObjectiveValue};
}

std::pair<std::vector<Takeoff>, double> completeOptimally(Schedule &aSchedule, std::atomic_bool &aKillSwitch) {
    std::unordered_set<Takeoff> myIgnoreTakeoffs{};
    auto myResults = recursiveFindOptimalCompletion(aSchedule, myIgnoreTakeoffs, aKillSwitch);
    const auto &myTakeoffs = std::get<0>(myResults);

    for (const auto &myTakeoff: myTakeoffs) {
        aSchedule.insertTakeoff(myTakeoff);
    }

    return myResults;
}
