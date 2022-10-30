#include "Grasp.h"
#include "input_parser/InputFormat.h"
#include "input_parser/InputParserFactory.h"
#include "InstanceConverter.h"
#include "Parameters.h"
#include "Statistics.h"

#include <atomic>
#include <chrono>
#include <CLI11.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>


using ordered_json = nlohmann::ordered_json;

namespace objective {
    void to_json(ordered_json &j, const Components &c) {
        j = ordered_json{
                {"wsn", c.theNegativeSum},
                {"z",   c.theMinSurplus},
                {"wo",  c.theTotalWaterOutput},
                {"obj", c.theObjective},
        };
    }
}

namespace local_search {
    void to_json(ordered_json &j, const ObjectiveComponentsSnapshot &ocs) {
        j = ordered_json{
                {"iter", ocs.theIteration},
                {"wsn",  ocs.theNegativeSum},
                {"z",    ocs.theMinSurplus},
                {"wo",   ocs.theTotalWaterOutput},
                {"obj",  ocs.theObjective},
        };
    }
}

void to_json(ordered_json &j, const Parameters &p) {
    j = ordered_json{
            {"_input",     p.theInputFilename},
            {"_output",    p.theOutputFilename},
            {"_threads",   p.theThreadCount},
            {"_timeout",   p.theTimeoutSeconds},
            {"_iters",     p.theGraspIterationsCount},
            {"_ls_iters",  p.theLsIterationsCount},
            {"_nd",        p.theNumberDestroy},
            {"_alpha_g",   p.theAlphaGreedy},
            {"_alpha_d",   p.theAlphaDestroy},
            {"_t0",        p.theT0},
            {"_temp_coef", p.theTempCoef},
    };
}

void assignCLI(CLI::App &app, Parameters &p) {
    app.add_option(
            "-i,--input",
            p.theInputFilename,
            "Path to input file. Reads from stdin if omitted or set to a dash symbol."
    )->required(false);

    app.add_option(
            "-o,--output",
            p.theOutputFilename,
            "Path to output file. Skips printing to a file if omitted or set to a dash symbol."
    )->required(false);

    static std::map<std::string, InputFormat> myInputFormatMap{
            {"simple", InputFormat::Raw},
            {"ampl",   InputFormat::Ampl}};
    app.add_option("-f,--format", p.theInputFormat, "Input data format.")
            ->required(false)
            ->transform(CLI::CheckedTransformer(myInputFormatMap, CLI::ignore_case));

    app.add_option(
            "-t,--threads",
            p.theThreadCount,
            "Number of worker threads."
    )->required(false);

    app.add_option(
            "--timeout",
            p.theTimeoutSeconds,
            "Execution time limit in seconds. Ignored if set to a negative value."
    )->required(false);

    app.add_option(
            "--iters",
            p.theGraspIterationsCount,
            "Total number of GRASP iterations to run. Preferably a multiple of --threads for maximum efficiency."
    )->required(false);

    app.add_option(
            "--ls-iters",
            p.theLsIterationsCount,
            "Number of iterations in local search."
    )->required(false);

    app.add_option(
            "--nd",
            p.theNumberDestroy,
            "Maximum number of takeoffs to be removed in destroy method. Actual number randomly chosen from range [1, N_D] in each iteration."
    )->required(false);

    app.add_option(
            "--alpha-g",
            p.theAlphaGreedy,
            "Alpha value for RCL in greedy construction and repair method."
    )->required(false);

    app.add_option(
            "--alpha-d",
            p.theAlphaDestroy,
            "Alpha value for RCL in destroy method."
    )->required(false);

    app.add_option(
            "--t0",
            p.theT0,
            "Starting temperature T0 in SA."
    )->required(false);

    app.add_option(
            "--temp-coef",
            p.theTempCoef,
            "Cooling factor in SA."
    )->required(false);
}

int main(int argc, char *argv[]) {
    const auto myStartTime = std::chrono::steady_clock::now();

    CLI::App myApp{"Aerial Resource Scheduler"};
    Parameters myParameters{};
    assignCLI(myApp, myParameters);
    CLI11_PARSE(myApp, argc, argv)

    std::istream *myInputStream;
    std::ifstream myInputFile;

    if (myParameters.theInputFilename == STDIN_STDOUT_FILENAME) {
        myInputStream = &std::cin;
    } else {
        myInputFile.open(myParameters.theInputFilename);

        if (myInputFile.fail()) {
            std::cout << "FATAL: Failed to open file " << myParameters.theInputFilename << std::endl;
            throw std::runtime_error("Failed to open file");
        }

        myInputStream = &myInputFile;
    }

    std::ofstream myOutputFile;
    if (myParameters.theOutputFilename != STDIN_STDOUT_FILENAME) {
        myOutputFile.open(myParameters.theOutputFilename);
        if (myOutputFile.fail()) {
            std::cout << "FATAL: Failed to open file " << myParameters.theOutputFilename << std::endl;
            throw std::runtime_error("Failed to open file");
        }
    }

    const auto myParser = InputParserFactory(myParameters.theInputFormat).getParser();
    const auto myInstanceRaw = myParser->parse(*myInputStream);
    const auto myInstance = convertInstance(myInstanceRaw);

    std::cout << "Finished parsing input!" << std::endl;

    const std::chrono::duration<double> myTimeout(
            myParameters.theTimeoutSeconds > 0 ? myParameters.theTimeoutSeconds : DBL_MAX
    );

    std::atomic_bool myKillSwitch{false};
    std::atomic_bool myKillTimer{false};
    std::thread myTimeoutThread([myStartTime, myTimeout, &myKillSwitch, &myKillTimer] {
        const auto myEndTime = myTimeout + myStartTime;
        while (!myKillTimer) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (std::chrono::steady_clock::now() > myEndTime) {
                myKillSwitch = true;
                break;
            }
        }
    });

    const auto myAccumulator =
            myParameters.theThreadCount > 1
            ? grasp::searchParallelized(&myInstance, myParameters, myKillSwitch)
            : grasp::searchSequential(&myInstance, myParameters, myKillSwitch);

    const std::chrono::duration<double> myElapsedSeconds = std::chrono::steady_clock::now() - myStartTime;

    myKillTimer = true;
    myTimeoutThread.join();

    const auto myGreedyBestObjective = *std::max_element(
            myAccumulator.theGreedyObjectiveComponents.begin(), myAccumulator.theGreedyObjectiveComponents.end(),
            [](const objective::Components &c1, const objective::Components &c2) {
                return c1.theObjective < c2.theObjective;
            });
    const auto[myGreedyMean, myGreedyStdDev] = statistics::meanAndDevTransform(
            myAccumulator.theGreedyObjectiveComponents);
    const auto[myLsMean, myLsStdDev] = statistics::meanAndDevTransform(
            myAccumulator.theLsObjectiveComponents);
    const auto[myIterationDurationMean, myIterationDurationStdDev] = statistics::meanAndDev(
            myAccumulator.theIterationDurations);
    const auto[myGreedyDurationMean, myGreedyDurationStdDev] = statistics::meanAndDev(
            myAccumulator.theGreedyDurations);
    const auto[myLsDurationMean, myLsDurationStdDev] = statistics::meanAndDev(myAccumulator.theLsDurations);

    ordered_json myOutputDataOverviewJson = {
            {"WO",                         (std::int64_t) round(myAccumulator.theBestSchedule.getTotalWaterOutput())},
            {"Sum_WSn",                    round(myAccumulator.theBestSchedule.getNegativeSurplusSum() * 1e2) / 1e2},
            {"Z",                          round(myAccumulator.theBestSchedule.getMinimumSurplus() * 1e2) / 1e2},
            {"objective",                  myAccumulator.theBestObjective},
            {"_solve_wall_time",           myElapsedSeconds.count()},
            {"_takeoffs_count",            myAccumulator.theBestSchedule.getTakeoffsCount()},
            {"_takeoffs_count_max",        myAccumulator.theBestSchedule.getInstance().theMaxTakeoffsCount},
            {"_drops_count",               round(myAccumulator.theBestSchedule.getTotalDropsCount() * 1e2) / 1e2},
            {"_best_iteration",            myAccumulator.theBestIterationIndex},
            {"_total_iterations",          myAccumulator.theCompletedGraspIterationsCount},
            {"_parameters",                myParameters},
            {"_objective_greedy_best",     myGreedyBestObjective},
            {"_objective_greedy_avg",      myGreedyMean},
            {"_objective_greedy_stddev",   myGreedyStdDev},
            {"_objective_ls_avg",          myLsMean},
            {"_objective_ls_stddev",       myLsStdDev},
            {"_duration_iteration_avg",    myIterationDurationMean},
            {"_duration_iteration_stddev", myIterationDurationStdDev},
            {"_duration_greedy_avg",       myGreedyDurationMean},
            {"_duration_greedy_stddev",    myGreedyDurationStdDev},
            {"_duration_ls_avg",           myLsDurationMean},
            {"_duration_ls_stddev",        myLsDurationStdDev},
            {"_durations_iteration",       myAccumulator.theIterationDurations},
            {"_durations_greedy",          myAccumulator.theGreedyDurations},
            {"_durations_ls",              myAccumulator.theLsDurations},
            {"_all_objectives_greedy",     myAccumulator.theGreedyObjectiveComponents},
            {"_all_objectives_ls",         myAccumulator.theLsObjectiveComponents},
            {"_annealing_snapshots",       myAccumulator.theBestObjectiveSnapshots},
    };

    std::string myOutputDataOverview = myOutputDataOverviewJson.dump(2);

    std::ostringstream myOutputDataSchedule;
    myOutputDataSchedule
            << std::fixed
            << std::setprecision(2)
            << "Condensed schedule:\n"
            << myAccumulator.theBestSchedule.buildFullScheduleCondensedMatrix()
            << "\n\nFull schedule:\n" << myAccumulator.theBestSchedule.buildFullScheduleMatrix()
            << "\nTakeoffs:\n" << myAccumulator.theBestSchedule.buildTakeoffsMatrix()
            << "\nWS:\n" << myAccumulator.theBestSchedule.getWaterTargetSurplus();

    std::cout << "\n\n" << myOutputDataSchedule.str() << "\n" << myOutputDataOverview << std::endl;
    if (myOutputFile.is_open()) {
        myOutputFile << myOutputDataOverview << "\n\n" << myOutputDataSchedule.str() << std::endl;
    }

    return 0;
}
