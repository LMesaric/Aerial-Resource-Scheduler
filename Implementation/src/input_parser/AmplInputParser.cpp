#include "AmplInputParser.h"

#include <cstdint>
#include <iostream>
#include <regex>

#define REPEAT(cnt) for (int x = 0; x < (cnt); ++x)


namespace {
    std::string extractLastNumberStr(const std::string &aLine) {
        const std::regex myRegex(R"(.*[A-Za-z\s]([\d\.]+)\s*;\s*)");
        std::smatch myMatch;
        std::regex_match(aLine, myMatch, myRegex);
        return myMatch[1].str();
    }

    template<typename T>
    std::vector<T> parseOneDimensionMap(std::istream &aStream, std::size_t aCount) {
        std::vector<T> myVector;
        myVector.resize(aCount);

        std::string myDump;
        for (std::size_t i = 0; i < aCount; ++i) {
            aStream >> myDump >> myVector[i];
        }

        std::getline(aStream, myDump);
        return myVector;
    }

    template<typename T>
    std::vector<std::vector<T>> parseTwoDimensionMap(std::istream &aStream, std::size_t aRowCnt, std::size_t aColCnt) {
        std::vector<std::vector<T>> myVector{aRowCnt, std::vector<T>(aColCnt)};

        std::string myDump;
        for (std::size_t i = 0; i < aRowCnt; ++i) {
            aStream >> myDump;
            for (std::size_t j = 0; j < aColCnt; ++j) {
                T myTmp;
                aStream >> myTmp;
                myVector[i][j] = std::move(myTmp);
            }
        }

        std::getline(aStream, myDump);
        return myVector;
    }

    template<typename T>
    void parseSingleParamAllVehicles(std::istream &aStream, InstanceRaw &anInstanceRaw, T VehicleRaw::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        std::vector<T> myMap = parseOneDimensionMap<T>(aStream, anInstanceRaw.getVehiclesCnt());
        for (std::size_t i = 0; i < anInstanceRaw.getVehiclesCnt(); ++i) {
            anInstanceRaw.theVehicles[i].*field = myMap[i];
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(std::istream &aStream, InstanceRaw &anInstanceRaw,
                                   std::vector<std::vector<T>> VehicleRaw::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        for (auto &myVehicle: anInstanceRaw.theVehicles) {
            (myVehicle.*field).resize(anInstanceRaw.getFrontsCnt());
        }
        for (std::size_t i = 0; i < anInstanceRaw.getFrontsCnt(); ++i) {
            REPEAT(2) std::getline(aStream, myLine); // blank; header

            auto myDownloads = parseTwoDimensionMap<T>(
                    aStream, anInstanceRaw.theTimeSlotsCount, anInstanceRaw.getVehiclesCnt());
            for (std::size_t j = 0; j < anInstanceRaw.getVehiclesCnt(); ++j) {
                for (std::size_t k = 0; k < anInstanceRaw.theTimeSlotsCount; ++k) {
                    (anInstanceRaw.theVehicles[j].*field)[i].push_back(myDownloads[k][j]);
                }
            }
        }
    }
}

InstanceRaw AmplInputParser::parse(std::istream &aStream) const {
    InstanceRaw myInstanceRaw{};

    std::string myLine;

    bool myFailedConversion = false;
    try {
        REPEAT(2) std::getline(aStream, myLine); // data; set K
        myInstanceRaw.theVehicles.resize(std::stoul(extractLastNumberStr(myLine)));

        REPEAT(2) std::getline(aStream, myLine); // blank; set F
        myInstanceRaw.theFronts.resize(std::stoul(extractLastNumberStr(myLine)));

        REPEAT(4) std::getline(aStream, myLine); // blank; set Q; blank; param T
        myInstanceRaw.theTimeSlotsCount = std::stoul(extractLastNumberStr(myLine));

        REPEAT(2) std::getline(aStream, myLine); // param V; header
        auto myIsHelicopter = parseTwoDimensionMap<bool>(aStream, 2, myInstanceRaw.getVehiclesCnt());
        for (std::size_t i = 0; i < myInstanceRaw.getVehiclesCnt(); ++i) {
            myInstanceRaw.theVehicles[i].theIsHelicopter = myIsHelicopter[0][i];
        }

        parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theFlightDurationLimit);
        parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theRestLimit);
        parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::thePilotPresenceLimit);
        parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theFlightCountLimit);

        REPEAT(4) std::getline(aStream, myLine); // comment; blank; param A; header

        auto myAvailability = parseTwoDimensionMap<bool>(
                aStream, myInstanceRaw.theTimeSlotsCount, myInstanceRaw.getVehiclesCnt());
        for (const auto &myTimeSlot: myAvailability) {
            for (std::size_t i = 0; i < myInstanceRaw.getVehiclesCnt(); ++i) {
                myInstanceRaw.theVehicles[i].theAvailability.push_back(myTimeSlot[i]);
            }
        }

        REPEAT(4) std::getline(aStream, myLine); // comment; blank; param B; header

        auto myIsOnlyHelicopters = parseTwoDimensionMap<bool>(aStream, 2, myInstanceRaw.getFrontsCnt());
        for (std::size_t i = 0; i < myInstanceRaw.getFrontsCnt(); ++i) {
            myInstanceRaw.theFronts[i].theIsOnlyHelicopters = myIsOnlyHelicopters[0][i];
        }

        REPEAT(4) std::getline(aStream, myLine); // comment; blank; param U; header

        auto myTransitTime = parseTwoDimensionMap<std::uint32_t>(
                aStream, myInstanceRaw.getVehiclesCnt(), myInstanceRaw.getFrontsCnt());
        for (std::size_t i = 0; i < myInstanceRaw.getVehiclesCnt(); ++i) {
            for (std::size_t j = 0; j < myInstanceRaw.getFrontsCnt(); ++j) {
                myInstanceRaw.theVehicles[i].theTransitTimes.push_back(myTransitTime[i][j]);
            }
        }

        parseSingleParamAllVehicles<std::uint32_t>(aStream, myInstanceRaw, &VehicleRaw::theWaterCapacity);

        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param S
        auto mySimultaneousResourcesLimit = parseOneDimensionMap<std::uint32_t>(aStream, myInstanceRaw.getFrontsCnt());
        for (std::size_t i = 0; i < myInstanceRaw.getFrontsCnt(); ++i) {
            myInstanceRaw.theFronts[i].theSimultaneousResourcesLimit = mySimultaneousResourcesLimit[i];
        }

        parseDownloadsAllVehicles<double>(aStream, myInstanceRaw, &VehicleRaw::theIntermediateDownloadsCount);
        parseDownloadsAllVehicles<double>(aStream, myInstanceRaw, &VehicleRaw::theTransitDownloadsCount);

        REPEAT(4) std::getline(aStream, myLine); // comment; blank; param W; header

        auto myTargetWaterContent = parseTwoDimensionMap<double>(
                aStream, myInstanceRaw.theTimeSlotsCount, myInstanceRaw.getFrontsCnt());

        for (std::size_t i = 0; i < myInstanceRaw.getFrontsCnt(); ++i) {
            for (std::size_t j = 0; j < myInstanceRaw.theTimeSlotsCount; ++j) {
                myInstanceRaw.theFronts[i].theTargetWaterContent.push_back(myTargetWaterContent[j][i]);
            }
        }

        REPEAT(5) std::getline(aStream, myLine); // comment; blank; param M; blank; param a1
        myInstanceRaw.theA1 = std::stod(extractLastNumberStr(myLine));

        REPEAT(2) std::getline(aStream, myLine); // blank; param a2
        myInstanceRaw.theA2 = std::stod(extractLastNumberStr(myLine));

        REPEAT(2) std::getline(aStream, myLine); // blank; param a3
        myInstanceRaw.theA3 = std::stod(extractLastNumberStr(myLine));

    } catch (...) {
        myFailedConversion = true;
    }

    if (aStream.fail() || myFailedConversion) {
        const auto myErrMsg = "Failed to correctly read input";
        std::cout << "FATAL: " << myErrMsg << std::endl;
        throw std::runtime_error(myErrMsg);
    }

    return myInstanceRaw;
}
