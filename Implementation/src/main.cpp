#include "Grasp.h"
#include "input_parser/InputFormat.h"
#include "input_parser/InputParserFactory.h"
#include "InstanceConverter.h"
#include "Parameters.h"
#include "Statistics.h"

#include <atomic>
#include <chrono>
#include <CLI11.hpp>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>


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
            "Alpha value for RCL in greedy construction."
    )->required(false);

    app.add_option(
            "--alpha-d",
            p.theAlphaDestroy,
            "Alpha value for RCL in destroy method."
    )->required(false);

    app.add_option(
            "--alpha-r",
            p.theAlphaRepair,
            "Alpha value for RCL in greedy part of repair method."
    )->required(false);

    app.add_option(
            "--kr-g",
            p.theK1Greedy,
            "Kr weight in greedy construction."
    )->required(false);

    app.add_option(
            "--kp-g",
            p.theK2Greedy,
            "Kp weight in greedy construction."
    )->required(false);

    app.add_option(
            "--kr-r",
            p.theK1Repair,
            "Kr weight in greedy part of repair method."
    )->required(false);

    app.add_option(
            "--kp-r",
            p.theK2Repair,
            "Kp weight in greedy part of repair method."
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

template<typename T>
std::string concatenate(const std::vector<T> &aVec, const std::string &aDelimiter = " ") {
    std::ostringstream myOutputStream;
    copy(aVec.begin(), aVec.end(), std::ostream_iterator<T>(myOutputStream, aDelimiter.c_str()));
    return myOutputStream.str();
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

    // const auto myAccumulator = grasp::searchSequential(&myInstance, myParameters, myKillSwitch);
    const auto myAccumulator = grasp::searchParallelized(&myInstance, myParameters, myKillSwitch);

    const std::chrono::duration<double> myElapsedSeconds = std::chrono::steady_clock::now() - myStartTime;

    myKillTimer = true;
    myTimeoutThread.join();

    const auto myGreedyBestObjective = *std::max_element(
            myAccumulator.theGreedyObjectives.begin(), myAccumulator.theGreedyObjectives.end());
    const auto [myGreedyMean, myGreedyStdDev] = statistics::meanAndDev(myAccumulator.theGreedyObjectives);
    const auto [myLsMean, myLsStdDev] = statistics::meanAndDev(myAccumulator.theLsObjectives);
    const auto [myIterationDurationMean, myIterationDurationStdDev] = statistics::meanAndDev(
            myAccumulator.theIterationDurations);
    const auto [myGreedyDurationMean, myGreedyDurationStdDev] = statistics::meanAndDev(
            myAccumulator.theGreedyDurations);
    const auto [myLsDurationMean, myLsDurationStdDev] = statistics::meanAndDev(myAccumulator.theLsDurations);

    std::ostringstream myOutputDataOverview;
    myOutputDataOverview
            << std::fixed
            << std::setprecision(0)
            << "WO = " << myAccumulator.theBestSchedule.getTotalWaterOutput()
            << std::setprecision(2)
            << "\nSum_WSn = " << myAccumulator.theBestSchedule.getNegativeSurplusSum()
            << "\nZ = " << myAccumulator.theBestSchedule.getMinimumSurplus()
            << std::setprecision(13)
            << "\nobjective = " << myAccumulator.theBestObjective
            << std::setprecision(6)
            << "\n_solve_wall_time = " << myElapsedSeconds.count()
            << "\n_takeoffs_count = " << myAccumulator.theBestSchedule.getTakeoffsCount()
            << "\n_takeoffs_count_max = "
            << myAccumulator.theBestSchedule.getInstance().theMaxTakeoffsCount
            << "\n_best_iteration = " << myAccumulator.theBestIterationIndex
            << "\n_total_iterations = " << myAccumulator.theCompletedGraspIterationsCount
            << "\n_threads = " << myParameters.theThreadCount
            << "\n_objective_greedy_best = " << myGreedyBestObjective
            << "\n_objective_greedy_avg = " << myGreedyMean
            << "\n_objective_greedy_stddev = " << myGreedyStdDev
            << "\n_objective_ls_avg = " << myLsMean
            << "\n_objective_ls_stddev = " << myLsStdDev
            << "\n_duration_iteration_avg = " << myIterationDurationMean
            << "\n_duration_iteration_stddev = " << myIterationDurationStdDev
            << "\n_duration_greedy_avg = " << myGreedyDurationMean
            << "\n_duration_greedy_stddev = " << myGreedyDurationStdDev
            << "\n_duration_ls_avg = " << myLsDurationMean
            << "\n_duration_ls_stddev = " << myLsDurationStdDev
            << "\n_durations_iteration = " << concatenate(myAccumulator.theIterationDurations)
            << "\n_durations_greedy = " << concatenate(myAccumulator.theGreedyDurations)
            << "\n_durations_ls = " << concatenate(myAccumulator.theLsDurations);

    std::ostringstream myOutputDataSchedule;
    myOutputDataSchedule
            << std::fixed
            << std::setprecision(2)
            << "Condensed schedule:\n"
            << myAccumulator.theBestSchedule.buildFullScheduleCondensedMatrix()
            << "\n\nFull schedule:\n" << myAccumulator.theBestSchedule.buildFullScheduleMatrix()
            << "\nTakeoffs:\n" << myAccumulator.theBestSchedule.buildTakeoffsMatrix()
            << "\nWS:\n" << myAccumulator.theBestSchedule.getWaterTargetSurplus();

    std::cout << "\n\n" << myOutputDataSchedule.str() << "\n" << myOutputDataOverview.str() << std::endl;
    if (myOutputFile.is_open()) {
        myOutputFile << myOutputDataOverview.str() << "\n\n" << myOutputDataSchedule.str() << std::endl;
    }

    return 0;
}
