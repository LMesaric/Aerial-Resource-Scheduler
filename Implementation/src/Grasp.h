#pragma once

#include "BS_thread_pool.hpp"
#include "Greedy.h"
#include "LocalSearch.h"
#include "Objective.h"
#include "Parameters.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <limits>
#include <list>
#include <thread>


namespace grasp {
    struct IterationResult {
        std::uint32_t theIterationIndex{};
        Schedule theSchedule;
        std::vector<local_search::ObjectiveComponentsSnapshot> theBestObjectiveSnapshots{};
        objective::Components theGreedyObjectiveComponents{};
        double theIterationDurationSeconds{};
        double theGreedyDurationSeconds{};
        double theLsDurationSeconds{};
    };

    struct Accumulator {
        Schedule theBestSchedule{};
        double theBestObjective{std::numeric_limits<double>::lowest()};

        std::uint32_t theBestIterationIndex{};
        std::uint32_t theCompletedGraspIterationsCount{};

        std::vector<std::vector<local_search::ObjectiveComponentsSnapshot>> theBestObjectiveSnapshots{};
        std::vector<objective::Components> theGreedyObjectiveComponents{};
        std::vector<objective::Components> theLsObjectiveComponents{};
        std::vector<double> theIterationDurations{};
        std::vector<double> theGreedyDurations{};
        std::vector<double> theLsDurations{};

        bool updateSchedule(IterationResult aResult) {
            ++theCompletedGraspIterationsCount;

            const auto myLsObjectiveComponents = objective::Components{aResult.theSchedule};

            std::cout << "Greedy: " << aResult.theGreedyObjectiveComponents.theObjective
                      << " -> LS: " << myLsObjectiveComponents.theObjective;

            theBestObjectiveSnapshots.push_back(std::move(aResult.theBestObjectiveSnapshots));
            theGreedyObjectiveComponents.push_back(aResult.theGreedyObjectiveComponents);
            theLsObjectiveComponents.push_back(myLsObjectiveComponents);
            theIterationDurations.push_back(aResult.theIterationDurationSeconds);
            theGreedyDurations.push_back(aResult.theGreedyDurationSeconds);
            theLsDurations.push_back(aResult.theLsDurationSeconds);

            if (myLsObjectiveComponents.theObjective > theBestObjective) {
                theBestIterationIndex = aResult.theIterationIndex;
                theBestObjective = myLsObjectiveComponents.theObjective;
                theBestSchedule = std::move(aResult.theSchedule);

                std::cout << " <<< New best" << std::endl;
                return true;
            } else {
                std::cout << std::endl;
                return false;
            }
        }
    };

    [[nodiscard]] IterationResult iteration(
            std::uint32_t anIterationIdx,
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_bool &aKillSwitch
    ) {
        const auto myIterationStartTime = std::chrono::steady_clock::now();

        std::random_device myRandomDevice{};
        std::mt19937 myGenerator{myRandomDevice()};
        IterationResult myResult{.theIterationIndex = anIterationIdx};

        Schedule myGreedySchedule = greedy::createGreedySchedule(
                anInstance,
                aKillSwitch,
                aParameters.theAlphaGreedy,
                aParameters.theFitnessWeightFactor,
                myGenerator
        );
        myResult.theGreedyObjectiveComponents = objective::Components{myGreedySchedule};

        const auto myIterationMiddleTime = std::chrono::steady_clock::now();

        auto [tmpSchedule_, tmpBestObjectiveSnapshots_] = local_search::search(
                std::move(myGreedySchedule), aParameters, myGenerator, aKillSwitch);
        myResult.theSchedule = std::move(tmpSchedule_);
        myResult.theBestObjectiveSnapshots = std::move(tmpBestObjectiveSnapshots_);

        const auto myIterationEndTime = std::chrono::steady_clock::now();

        const std::chrono::duration<double> myIterationDuration = myIterationEndTime - myIterationStartTime;
        myResult.theIterationDurationSeconds = myIterationDuration.count();

        const std::chrono::duration<double> myGreedyDuration = myIterationMiddleTime - myIterationStartTime;
        myResult.theGreedyDurationSeconds = myGreedyDuration.count();

        const std::chrono::duration<double> myLsDuration = myIterationEndTime - myIterationMiddleTime;
        myResult.theLsDurationSeconds = myLsDuration.count();

        return myResult;
    }

    [[nodiscard]] Accumulator searchSequential(
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_bool &aKillSwitch
    ) {
        Accumulator myAccumulator{};

        for (auto i = 0; i < aParameters.theGraspIterationsCount && !aKillSwitch; ++i) {
            auto myIterationResult = iteration(i, anInstance, aParameters, aKillSwitch);
            myAccumulator.updateSchedule(std::move(myIterationResult));
        }

        return myAccumulator;
    }

    [[nodiscard]] Accumulator searchParallelized(
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_bool &aKillSwitch
    ) {
        const std::uint32_t myHardwareThreadCnt = std::thread::hardware_concurrency();
        const std::uint32_t myThreadPoolSize = std::min(
                {aParameters.theThreadCount,
                 aParameters.theGraspIterationsCount,
                 myHardwareThreadCnt == 0 ? std::numeric_limits<std::uint32_t>::max() : myHardwareThreadCnt}
        );

        BS::thread_pool myPool{myThreadPoolSize};
        std::list<std::future<IterationResult>> myFutures;

        std::uint32_t mySubmittedCount{0};

        for (; mySubmittedCount < myThreadPoolSize; ++mySubmittedCount) {
            myFutures.emplace_back(
                    myPool.submit(iteration, mySubmittedCount, anInstance, aParameters, std::ref(aKillSwitch))
            );
        }

        Accumulator myAccumulator{};

        while (!myFutures.empty()) {
            for (auto myIt = myFutures.begin(); myIt != myFutures.end();) {
                if (myIt->wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout) {
                    myIt++;
                    continue;
                }

                myAccumulator.updateSchedule(std::move(myIt->get()));
                myIt = myFutures.erase(myIt);

                if (!aKillSwitch && mySubmittedCount < aParameters.theGraspIterationsCount) {
                    myFutures.emplace_back(myPool.submit(
                            iteration, mySubmittedCount++, anInstance, aParameters, std::ref(aKillSwitch)
                    ));
                }
            }
        }

        return myAccumulator;
    }
}
