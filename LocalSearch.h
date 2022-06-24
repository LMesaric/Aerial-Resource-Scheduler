#pragma once

#include "Greedy.h"
#include "Instance.h"
#include "Objective.h"
#include "Optimal.h"
#include "Parameters.h"
#include "Schedule.h"

#include <cmath>
#include <memory>
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

        const auto [myMinIt, myMaxIt] = std::minmax_element(myTakeoffsObjectives.begin(), myTakeoffsObjectives.end());
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
    Schedule search(Schedule aSchedule, const Parameters &aParameters, std::mt19937 &aGenerator) {
        Schedule myCurrentSchedule{std::move(aSchedule)};
        double myCurrentObjective{evaluateObjective(myCurrentSchedule)};

        Schedule myBestSchedule{myCurrentSchedule};
        double myBestObjective{myCurrentObjective};

        std::uint32_t myBestIteration = 0;

        double myTemperature = aParameters.theT0;

        std::vector<Takeoff> myRemovedTakeoffs{};
        myRemovedTakeoffs.reserve(aParameters.theNumberDestroy);

        std::vector<Takeoff> myInsertedGreedyTakeoffs{};
        myInsertedGreedyTakeoffs.reserve(aParameters.theNumberRepair);

        for (std::uint32_t myIterationCount = 1;
             myIterationCount <= aParameters.theLsIterationsCount;
             ++myIterationCount) {

            for (int i = 0; i < aParameters.theNumberDestroy && myCurrentSchedule.getTakeoffsCount() > 0; ++i) {
                Takeoff myRemovedTakeoff = destroyOneTakeoff(
                        myCurrentSchedule, aParameters.theAlphaDestroy, aGenerator);
                myRemovedTakeoffs.push_back(myRemovedTakeoff);
            }

            for (int i = 0; i < aParameters.theNumberRepair; ++i) {
                std::optional<Takeoff> myInsertedGreedyTakeoff = greedy::pickGreedyTakeoff(
                        myCurrentSchedule,
                        aParameters.theAlphaRepair,
                        aParameters.theK1Repair,
                        aParameters.theK2Repair,
                        aGenerator
                );
                if (!myInsertedGreedyTakeoff) {
                    break;
                }

                myInsertedGreedyTakeoffs.push_back(*myInsertedGreedyTakeoff);
                myCurrentSchedule.insertTakeoff(*myInsertedGreedyTakeoff);
            }

            auto [myInsertedOptimalTakeoffs, myNewObjective] = completeOptimally(myCurrentSchedule);

            if (myNewObjective > myBestObjective) {
                myBestSchedule = myCurrentSchedule;
                myBestObjective = myNewObjective;
                myBestIteration = myIterationCount;
            }

            const double myDeltaCurrent = myNewObjective - myCurrentObjective;

            std::uniform_real_distribution<double> myDistribution(0.0, 1.0);
            const auto myRandomValue = myDistribution(aGenerator);

            if (myDeltaCurrent > 0 || myRandomValue < std::exp(myDeltaCurrent / myTemperature)) {
                myCurrentObjective = myNewObjective;
            } else {
                for (const auto &myInsertedTakeoff: myInsertedOptimalTakeoffs) {
                    myCurrentSchedule.removeTakeoff(myInsertedTakeoff);
                }
                for (const auto &myInsertedTakeoff: myInsertedGreedyTakeoffs) {
                    myCurrentSchedule.removeTakeoff(myInsertedTakeoff);
                }
                for (const auto &myRemovedTakeoff: myRemovedTakeoffs) {
                    myCurrentSchedule.insertTakeoff(myRemovedTakeoff);
                }
            }

            myInsertedGreedyTakeoffs.clear();
            myRemovedTakeoffs.clear();

            myTemperature *= aParameters.theTempCoeff;
        }

        return myBestSchedule;
    }
}
