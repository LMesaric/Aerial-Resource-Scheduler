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
    std::size_t pickRandomDestroyIndex(
            Schedule &aSchedule,
            const std::vector<Takeoff> &aTakeoffs,
            double anAlphaDestroy,
            std::mt19937 &aGenerator
    ) {
        if (anAlphaDestroy == 1.0) {
            std::uniform_int_distribution<std::size_t> myDistribution{0, aTakeoffs.size() - 1};
            return myDistribution(aGenerator);
        }

        std::vector<double> myTakeoffsObjectives{};
        myTakeoffsObjectives.reserve(aTakeoffs.size());

        for (const auto &myTakeoff: aTakeoffs) {
            aSchedule.removeTakeoff(myTakeoff);
            myTakeoffsObjectives.push_back(objective::evaluateObjective(aSchedule));
            aSchedule.insertTakeoff(myTakeoff);
        }

        return stochasticallySelectFromRcl(myTakeoffsObjectives, anAlphaDestroy, aGenerator);
    }

    Takeoff destroyOneTakeoff(Schedule &aSchedule, double anAlphaDestroy, std::mt19937 &aGenerator) {
        std::vector<Takeoff> myTakeoffs{aSchedule.getTakeoffs().begin(), aSchedule.getTakeoffs().end()};
        std::size_t myRandomIndex = pickRandomDestroyIndex(aSchedule, myTakeoffs, anAlphaDestroy, aGenerator);

        Takeoff myRemovedTakeoff = myTakeoffs[myRandomIndex];
        aSchedule.removeTakeoff(myRemovedTakeoff);
        return myRemovedTakeoff;
    }
}

namespace local_search {
    struct ObjectiveComponentsSnapshot : public objective::Components {
        std::uint32_t theIteration{};

        ObjectiveComponentsSnapshot() = default;

        ObjectiveComponentsSnapshot(std::uint32_t anIteration, const Schedule &aSchedule) :
                Components(aSchedule),
                theIteration(anIteration) {}
    };

    std::pair<Schedule, std::vector<ObjectiveComponentsSnapshot>> search(
            Schedule aSchedule,
            const Parameters &aParameters,
            std::mt19937 &aGenerator,
            std::atomic_bool &aKillSwitch
    ) {
        static constexpr auto mySnapshotSkip = 50;

        Schedule myCurrentSchedule{std::move(aSchedule)};
        double myCurrentObjective{objective::evaluateObjective(myCurrentSchedule)};

        Schedule myBestSchedule{myCurrentSchedule};
        double myBestObjective{myCurrentObjective};

        std::vector<ObjectiveComponentsSnapshot> myBestObjectiveSnapshots{};
        myBestObjectiveSnapshots.reserve(aParameters.theLsIterationsCount / mySnapshotSkip + 1);

        std::uint32_t myBestIteration = 0;

        double myTemperature = aParameters.theT0;

        std::vector<Takeoff> myRemovedTakeoffs{};
        myRemovedTakeoffs.reserve(aParameters.theNumberDestroy);

        std::vector<Takeoff> myInsertedGreedyTakeoffs{};
        myInsertedGreedyTakeoffs.reserve(aParameters.theNumberDestroy);

        for (std::uint32_t myIterationCount = 0;
             myIterationCount < aParameters.theLsIterationsCount && !aKillSwitch;
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

            const double myNewObjective = objective::evaluateObjective(myCurrentSchedule);

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

            if (myIterationCount % mySnapshotSkip == 0) {
                myBestObjectiveSnapshots.emplace_back(myIterationCount, myBestSchedule);
            }
        }

        return {std::move(myBestSchedule), std::move(myBestObjectiveSnapshots)};
    }
}
