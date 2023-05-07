#pragma once

#include "BS_thread_pool.hpp"
#include "Greedy.h"
#include "LocalSearch.h"
#include "Objective.h"
#include "Parameters.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
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

    struct ThreadSafeAccumulatorWrapper {
        Accumulator &theAccumulator;
        std::mutex theMutex{};

        explicit ThreadSafeAccumulatorWrapper(Accumulator &anAccumulator) : theAccumulator{anAccumulator} {}

        bool updateSchedule(IterationResult aResult) {
            const std::lock_guard lock{theMutex};
            return theAccumulator.updateSchedule(std::move(aResult));
        }
    };

    [[nodiscard]] IterationResult iteration(
            std::uint32_t anIterationIdx,
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_flag *aKillSwitch
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
                std::move(myGreedySchedule), aParameters, myGenerator, aKillSwitch
        );
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

    struct IterationTask {
        std::uint32_t theIterationIdx{};
        const Instance *theInstance;
        const Parameters &theParameters;
        std::atomic_flag *theKillSwitch;
        ThreadSafeAccumulatorWrapper &theAccumulator;

        void run() const {
            theAccumulator.updateSchedule(iteration(theIterationIdx, theInstance, theParameters, theKillSwitch));
        }
    };

    struct TaskGenerator {
        const std::uint32_t theTaskCount{};
        std::atomic_uint32_t theNextTaskIdx{0};

        const Instance *theInstance;
        const Parameters &theParameters;
        std::atomic_flag *theKillSwitch;
        ThreadSafeAccumulatorWrapper &theAccumulator;

        TaskGenerator(std::uint32_t aTaskCount,
                      const Instance *anInstance,
                      const Parameters &aParameters,
                      std::atomic_flag *aKillSwitch,
                      ThreadSafeAccumulatorWrapper &anAccumulator)
                : theTaskCount{aTaskCount},
                  theInstance{anInstance},
                  theParameters{aParameters},
                  theKillSwitch{aKillSwitch},
                  theAccumulator{anAccumulator} {}

        std::optional<IterationTask> generateTask() {
            if (theKillSwitch && theKillSwitch->test(std::memory_order_relaxed)) { return {}; }

            std::uint32_t myNextTaskIdx = theNextTaskIdx.fetch_add(1U, std::memory_order_relaxed);
            if (myNextTaskIdx >= theTaskCount) { return {}; }

            return IterationTask{
                    .theIterationIdx = myNextTaskIdx,
                    .theInstance = theInstance,
                    .theParameters = theParameters,
                    .theKillSwitch = theKillSwitch,
                    .theAccumulator = theAccumulator
            };
        }
    };

    [[nodiscard]] Accumulator searchSequential(
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_flag *aKillSwitch
    ) {
        Accumulator myAccumulator{};

        for (auto i = 0;
             i < aParameters.theGraspIterationsCount && !(aKillSwitch && aKillSwitch->test(std::memory_order_relaxed));
             ++i) {
            auto myIterationResult = iteration(i, anInstance, aParameters, aKillSwitch);
            myAccumulator.updateSchedule(std::move(myIterationResult));
        }

        return myAccumulator;
    }

    [[nodiscard]] Accumulator searchParallelized(
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_flag *aKillSwitch
    ) {
        const std::uint32_t myHardwareThreadCnt = std::thread::hardware_concurrency();
        const std::uint32_t myThreadPoolSize = std::min(
                {aParameters.theThreadCount,
                 aParameters.theGraspIterationsCount,
                 myHardwareThreadCnt == 0 ? std::numeric_limits<std::uint32_t>::max() : myHardwareThreadCnt}
        );

        BS::thread_pool myPool{myThreadPoolSize};
        Accumulator myAccumulator{};
        ThreadSafeAccumulatorWrapper myThreadSafeAccumulator{myAccumulator};
        TaskGenerator myTaskGenerator{
                aParameters.theGraspIterationsCount,
                anInstance,
                aParameters,
                aKillSwitch,
                myThreadSafeAccumulator
        };

        for (int i = 0; i < myThreadPoolSize; ++i) {
            myPool.push_task([&myTaskGenerator] {
                while (true) {
                    std::optional<IterationTask> myTask = myTaskGenerator.generateTask();
                    if (!myTask) {
                        break;
                    }
                    myTask->run();
                }
            });
        }

        myPool.wait_for_tasks();
        return myAccumulator;
    }

    [[nodiscard]] Accumulator search(
            const Instance *anInstance,
            const Parameters &aParameters,
            std::atomic_flag *aKillSwitch = nullptr
    ) {
        return aParameters.theThreadCount > 1
               ? searchParallelized(anInstance, aParameters, aKillSwitch)
               : searchSequential(anInstance, aParameters, aKillSwitch);
    }

    [[nodiscard]] Accumulator searchWithTimeout(
            const Instance *anInstance,
            const Parameters &aParameters,
            auto anEndTime
    ) {
        std::mutex myKillSwitchMutex;
        std::condition_variable myKillSwitchCv;
        std::atomic_flag myKillSwitch = ATOMIC_FLAG_INIT;
        std::jthread myTimeoutThread(
                [anEndTime, &myKillSwitch, &myKillSwitchMutex, &myKillSwitchCv] {
                    std::unique_lock myLock{myKillSwitchMutex};
                    myKillSwitchCv.wait_until(
                            myLock,
                            anEndTime,
                            [&myKillSwitch] { return myKillSwitch.test(std::memory_order_relaxed); }
                    );
                    myKillSwitch.test_and_set(std::memory_order_relaxed);
                }
        );

        Accumulator myAccumulator = search(anInstance, aParameters, &myKillSwitch);

        {
            std::lock_guard myLock{myKillSwitchMutex};
            myKillSwitch.test_and_set(std::memory_order_relaxed);
        }
        myKillSwitchCv.notify_all();

        return myAccumulator;
    }
}
