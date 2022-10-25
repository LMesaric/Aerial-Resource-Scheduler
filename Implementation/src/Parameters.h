#pragma once

#include "input_parser/InputFormat.h"

#include <cstdint>
#include <string>


static const auto STDIN_STDOUT_FILENAME = "-";

struct Parameters {
    std::string theInputFilename{STDIN_STDOUT_FILENAME};
    std::string theOutputFilename{STDIN_STDOUT_FILENAME};
    InputFormat theInputFormat{InputFormat::Ampl};

    std::uint32_t theThreadCount{1};
    double theTimeoutSeconds{-1};

    std::uint32_t theGraspIterationsCount{20};
    std::uint32_t theLsIterationsCount{6000};

    std::uint32_t theNumberDestroy{8};

    double theAlphaGreedy{0.1};
    double theAlphaDestroy{0.75};

    double theT0{1e12};
    double theTempCoef{0.995};
};
