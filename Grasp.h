#pragma once

#include "BS_thread_pool.hpp"
#include "Greedy.h"
#include "LocalSearch.h"
#include "Parameters.h"

#include <cfloat>
#include <chrono>
#include <future>
#include <list>
#include <memory>


namespace grasp {
    struct IterationResult {
        Schedule theSchedule;
        double theGreedyObjective{};
        double theIterationDurationSeconds{};
        double theGreedyDurationSeconds{};
        double theLsDurationSeconds{};
        std::uint32_t theIterationIndex{};
    };

    struct Accumulator {
        Schedule theBestSchedule{};
        double theBestObjective{-DBL_MAX};

        std::uint32_t theBestIterationIndex{};

        std::vector<double> theGreedyObjectives{};
        std::vector<double> theLsObjectives{};
        std::vector<double> theIterationDurations{};
        std::vector<double> theGreedyDurations{};
        std::vector<double> theLsDurations{};

        bool updateSchedule(IterationResult aResult) {
            const double myObjective = evaluateObjective(aResult.theSchedule);

            theGreedyObjectives.push_back(aResult.theGreedyObjective);
            theLsObjectives.push_back(myObjective);
            theIterationDurations.push_back(aResult.theIterationDurationSeconds);
            theGreedyDurations.push_back(aResult.theGreedyDurationSeconds);
            theLsDurations.push_back(aResult.theLsDurationSeconds);

            std::cout << "Greedy: " << aResult.theGreedyObjective << " -> LS: " << myObjective;

            if (myObjective > theBestObjective) {
                theBestIterationIndex = aResult.theIterationIndex;
                theBestObjective = myObjective;
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
            const std::shared_ptr<const Instance> &anInstance,
            const Parameters &aParameters
    ) {
        const auto myIterationStartTime = std::chrono::steady_clock::now();

        std::random_device myRandomDevice{};
        std::mt19937 myGenerator{myRandomDevice()};
        IterationResult myResult{.theIterationIndex = anIterationIdx};

        Schedule myGreedySchedule = greedy::createGreedySchedule(
                anInstance,
                aParameters.theAlphaRepair,
                aParameters.theK1Greedy,
                aParameters.theK2Greedy,
                myGenerator
        );
        myResult.theGreedyObjective = evaluateObjective(myGreedySchedule);

        const auto myMiddleTime = std::chrono::steady_clock::now();
        myResult.theSchedule = local_search::search(std::move(myGreedySchedule), aParameters, myGenerator);
        const auto myIterationEndTime = std::chrono::steady_clock::now();

        const std::chrono::duration<double> myIterationDuration = myIterationEndTime - myIterationStartTime;
        myResult.theIterationDurationSeconds = myIterationDuration.count();

        const std::chrono::duration<double> myGreedyDuration = myMiddleTime - myIterationStartTime;
        myResult.theGreedyDurationSeconds = myGreedyDuration.count();

        const std::chrono::duration<double> myLsDuration = myIterationEndTime - myMiddleTime;
        myResult.theLsDurationSeconds = myLsDuration.count();

        return myResult;
    }

    [[nodiscard]] Accumulator searchSequential(
            const std::shared_ptr<const Instance> &anInstance,
            const Parameters &aParameters
    ) {
        Accumulator myAccumulator{};

        for (auto i = 0; i < aParameters.theGraspIterationsCount; ++i) {
            auto myIterationResult = iteration(i, anInstance, aParameters);
            myAccumulator.updateSchedule(std::move(myIterationResult));
        }

        return myAccumulator;
    }

    [[nodiscard]] Accumulator searchParallelized(
            const std::shared_ptr<const Instance> &anInstance,
            const Parameters &aParameters
    ) {
        BS::thread_pool myPool{aParameters.theThreadCount};
        std::list<std::future<IterationResult>> myFutures;

        for (auto i = 0; i < aParameters.theGraspIterationsCount; ++i) {
            myFutures.push_back(myPool.submit(iteration, i, anInstance, aParameters));
        }

        Accumulator myAccumulator{};

        while (!myFutures.empty()) {
            for (auto myIt = myFutures.begin(); myIt != myFutures.end();) {
                if (myIt->wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout) {
                    myIt++;
                    continue;
                }

                auto myIterationResult = myIt->get();
                myAccumulator.updateSchedule(std::move(myIterationResult));
                myIt = myFutures.erase(myIt);
            }
        }

        return myAccumulator;
    }
}
