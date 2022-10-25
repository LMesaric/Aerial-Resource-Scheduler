#pragma once

#include "Greedy.h"
#include "Instance.h"
#include "Objective.h"
#include "Parameters.h"
#include "Schedule.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>


namespace {
    Takeoff destroyOneTakeoff(Schedule &aSchedule, double anAlphaDestroy, std::mt19937 &aGenerator) {
        std::vector<Takeoff> myTakeoffs{aSchedule.getTakeoffs().begin(), aSchedule.getTakeoffs().end()};
        std::vector<double> myTakeoffsObjectives{};
        myTakeoffsObjectives.reserve(myTakeoffs.size());

        for (const auto &myTakeoff: myTakeoffs) {
            aSchedule.removeTakeoff(myTakeoff);
            myTakeoffsObjectives.push_back(evaluateObjective(aSchedule));
            aSchedule.insertTakeoff(myTakeoff);
        }

        const auto[myMinIt, myMaxIt] = std::minmax_element(myTakeoffsObjectives.begin(), myTakeoffsObjectives.end());
        const double myMinFit = *myMinIt;
        const double myMaxFit = *myMaxIt;

        const double myThreshold = myMaxFit - anAlphaDestroy * (myMaxFit - myMinFit);

        std::vector<std::size_t> myRclIndices{};
        for (std::size_t i = 0; i < myTakeoffsObjectives.size(); ++i) {
            if (myTakeoffsObjectives[i] >= myThreshold) {
                myRclIndices.push_back(i);
            }
        }

        std::uniform_int_distribution<std::size_t> myDistribution{0, myRclIndices.size() - 1};
        const auto myRandomIndex = myDistribution(aGenerator);

        Takeoff myRemovedTakeoff = myTakeoffs[myRclIndices[myRandomIndex]];
        aSchedule.removeTakeoff(myRemovedTakeoff);

        return myRemovedTakeoff;
    }
}

namespace local_search {
    Schedule search(
            Schedule aSchedule,
            const Parameters &aParameters,
            std::mt19937 &aGenerator,
            std::atomic_bool &aKillSwitch
    ) {
        Schedule myCurrentSchedule{std::move(aSchedule)};
        double myCurrentObjective{evaluateObjective(myCurrentSchedule)};

        Schedule myBestSchedule{myCurrentSchedule};
        double myBestObjective{myCurrentObjective};

        std::uint32_t myBestIteration = 0;

        double myTemperature = aParameters.theT0;

        std::vector<Takeoff> myRemovedTakeoffs{};
        myRemovedTakeoffs.reserve(aParameters.theNumberDestroy);

        std::vector<Takeoff> myInsertedGreedyTakeoffs{};
        myInsertedGreedyTakeoffs.reserve(aParameters.theNumberDestroy);

        for (std::uint32_t myIterationCount = 1;
             myIterationCount <= aParameters.theLsIterationsCount && !aKillSwitch;
             ++myIterationCount) {

            std::uniform_int_distribution<std::uint32_t> myDestroyDistribution{
                    1,
                    std::min<std::uint32_t>(aParameters.theNumberDestroy, myCurrentSchedule.getTakeoffsCount())};
            const auto myRandomNumberDestroy = myDestroyDistribution(aGenerator);

            for (int i = 0; i < myRandomNumberDestroy; ++i) {
                Takeoff myRemovedTakeoff = destroyOneTakeoff(
                        myCurrentSchedule, aParameters.theAlphaDestroy, aGenerator);
                myRemovedTakeoffs.push_back(myRemovedTakeoff);
            }

            while (true) {
                std::optional<Takeoff> myInsertedGreedyTakeoff = greedy::pickGreedyTakeoff(
                        myCurrentSchedule,
                        aParameters.theAlphaGreedy,
                        aGenerator
                );
                if (!myInsertedGreedyTakeoff) {
                    break;
                }

                myInsertedGreedyTakeoffs.push_back(*myInsertedGreedyTakeoff);
                myCurrentSchedule.insertTakeoff(*myInsertedGreedyTakeoff);
            }

            const double myNewObjective = evaluateObjective(myCurrentSchedule);

            if (myNewObjective > myBestObjective) {
                myBestSchedule = myCurrentSchedule;
                myBestObjective = myNewObjective;
                myBestIteration = myIterationCount;
            }

            const double myDeltaCurrent = myNewObjective - myCurrentObjective;

            std::uniform_real_distribution<double> myAnnealingDistribution(0.0, 1.0);
            const auto myRandomValue = myAnnealingDistribution(aGenerator);

            if (myDeltaCurrent > 0 || myRandomValue < std::exp(myDeltaCurrent / myTemperature)) {
                myCurrentObjective = myNewObjective;
            } else {
                for (const auto &myInsertedTakeoff: myInsertedGreedyTakeoffs) {
                    myCurrentSchedule.removeTakeoff(myInsertedTakeoff);
                }
                for (const auto &myRemovedTakeoff: myRemovedTakeoffs) {
                    myCurrentSchedule.insertTakeoff(myRemovedTakeoff);
                }
            }

            myInsertedGreedyTakeoffs.clear();
            myRemovedTakeoffs.clear();

            myTemperature *= aParameters.theTempCoef;
        }

        return myBestSchedule;
    }
}
