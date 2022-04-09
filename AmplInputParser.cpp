#include "AmplInputParser.h"

#include <fstream>
#include <regex>

#define REPEAT(cnt) for (int x = 0; x < (cnt); ++x)


namespace {
    std::uint32_t parseLastNumber(const std::string &aLine) {
        const std::regex myRegex(R"(.*[A-Za-z\s](\d+)\s*;\s*)");
        std::smatch myMatch;
        std::regex_match(aLine, myMatch, myRegex);
        const std::string myCountRaw = myMatch[1].str();
        return std::stoul(myCountRaw);
    }

    template<typename T>
    std::vector<T> parseOneDimensionMap(std::istream &aStream, std::uint32_t aCount) {
        std::vector<T> myVector;
        myVector.resize(aCount);

        std::string myDump;
        for (int i = 0; i < aCount; ++i) {
            aStream >> myDump >> myVector[i];
        }

        std::getline(aStream, myDump);
        return myVector;
    }

    template<typename T>
    std::vector<std::vector<T>> parseTwoDimensionMap(
            std::istream &aStream, std::uint32_t aRowCnt, std::uint32_t aColCnt) {
        std::vector<std::vector<T>> myVector{aRowCnt, std::vector<T>(aColCnt)};

        std::string myDump;
        for (int i = 0; i < aRowCnt; ++i) {
            aStream >> myDump;
            for (int j = 0; j < aColCnt; ++j) {
                aStream >> myVector[i][j];
            }
        }

        std::getline(aStream, myDump);
        return myVector;
    }

    template<typename T>
    void parseSingleParamAllVehicles(std::istream &aStream, Parameters &aParameters, T Vehicle::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        std::vector<T> myMap = parseOneDimensionMap<T>(aStream, aParameters.getVehiclesCnt());
        for (int i = 0; i < aParameters.getVehiclesCnt(); ++i) {
            aParameters.theVehicles[i].*field = myMap[i];
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(std::istream &aStream, Parameters &aParameters,
                                   std::vector<std::vector<T>> Vehicle::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        for (auto &myVehicle: aParameters.theVehicles) {
            (myVehicle.*field).resize(aParameters.getFrontsCnt());
        }
        for (int i = 0; i < aParameters.getFrontsCnt(); ++i) {
            REPEAT(2) std::getline(aStream, myLine); // blank; header

            auto myDownloads = parseTwoDimensionMap<T>(
                    aStream, aParameters.theTimeSlotsCount, aParameters.getVehiclesCnt());
            for (int j = 0; j < aParameters.getVehiclesCnt(); ++j) {
                for (int k = 0; k < aParameters.theTimeSlotsCount; ++k) {
                    (aParameters.theVehicles[j].*field)[i].push_back(myDownloads[k][j]);
                }
            }
        }
    }
}

Parameters AmplInputParser::parse(std::istream &aStream) {
    Parameters myParameters{};

    std::string myLine;

    REPEAT(2) std::getline(aStream, myLine); // data; set K
    myParameters.theVehicles.resize(parseLastNumber(myLine));

    REPEAT(2) std::getline(aStream, myLine); // blank; set F
    myParameters.theFronts.resize(parseLastNumber(myLine));

    REPEAT(4) std::getline(aStream, myLine); // blank; set Q; blank; param T
    myParameters.theTimeSlotsCount = parseLastNumber(myLine);

    REPEAT(2) std::getline(aStream, myLine); // param V; header
    auto myIsHelicopter = parseTwoDimensionMap<int>(aStream, 2, myParameters.getVehiclesCnt());
    for (int i = 0; i < myParameters.getVehiclesCnt(); ++i) {
        myParameters.theVehicles[i].theIsHelicopter = static_cast<bool>(myIsHelicopter[0][i]);
    }

    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParameters, &Vehicle::theFlightDurationLimit);
    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParameters, &Vehicle::theRestLimit);
    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParameters, &Vehicle::thePilotPresenceLimit);
    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParameters, &Vehicle::theFlightCountLimit);

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param A; header

    auto myAvailability = parseTwoDimensionMap<int>(
            aStream, myParameters.theTimeSlotsCount, myParameters.getVehiclesCnt());
    for (const auto &myTimeSlot: myAvailability) {
        for (int i = 0; i < myParameters.getVehiclesCnt(); ++i) {
            myParameters.theVehicles[i].theAvailability.push_back(static_cast<char>(myTimeSlot[i]));
        }
    }

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param B; header

    auto myIsOnlyHelicopters = parseTwoDimensionMap<int>(aStream, 2, myParameters.getFrontsCnt());
    for (int i = 0; i < myParameters.getFrontsCnt(); ++i) {
        myParameters.theFronts[i].theIsOnlyHelicopters = static_cast<bool>(myIsOnlyHelicopters[0][i]);
    }

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param U; header

    auto myTransitTime = parseTwoDimensionMap<std::uint32_t>(
            aStream, myParameters.getVehiclesCnt(), myParameters.getFrontsCnt());
    for (int i = 0; i < myParameters.getVehiclesCnt(); ++i) {
        for (int j = 0; j < myParameters.getFrontsCnt(); ++j) {
            myParameters.theVehicles[i].theTransitTime.push_back(myTransitTime[i][j]);
        }
    }

    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParameters, &Vehicle::theWaterCapacity);

    REPEAT(3) std::getline(aStream, myLine); // comment; blank; param S
    auto mySimultaneousResourcesLimit = parseOneDimensionMap<std::uint32_t>(aStream, myParameters.getFrontsCnt());
    for (int i = 0; i < myParameters.getFrontsCnt(); ++i) {
        myParameters.theFronts[i].theSimultaneousResourcesLimit = mySimultaneousResourcesLimit[i];
    }

    parseDownloadsAllVehicles<double>(aStream, myParameters, &Vehicle::theIntermediateDownloads);
    parseDownloadsAllVehicles<double>(aStream, myParameters, &Vehicle::theTransitDownloads);

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param W; header

    auto myTargetWaterContent = parseTwoDimensionMap<double>(
            aStream, myParameters.theTimeSlotsCount, myParameters.getFrontsCnt());

    for (int i = 0; i < myParameters.getFrontsCnt(); ++i) {
        for (int j = 0; j < myParameters.theTimeSlotsCount; ++j) {
            myParameters.theFronts[i].theTargetWaterContent.push_back(myTargetWaterContent[j][i]);
        }
    }

    if (aStream.fail()) {
        throw std::runtime_error("Failed to correctly read input");
    }

    return myParameters;
}
