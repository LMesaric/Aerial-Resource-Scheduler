#include "AmplInputParser.h"

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
    void parseSingleParamAllVehicles(std::istream &aStream, ParametersRaw &aParametersRaw, T VehicleRaw::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        std::vector<T> myMap = parseOneDimensionMap<T>(aStream, aParametersRaw.getVehiclesCnt());
        for (std::size_t i = 0; i < aParametersRaw.getVehiclesCnt(); ++i) {
            aParametersRaw.theVehicles[i].*field = myMap[i];
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(std::istream &aStream, ParametersRaw &aParametersRaw,
                                   std::vector<std::vector<T>> VehicleRaw::* field) {
        std::string myLine;
        REPEAT(3) std::getline(aStream, myLine); // comment; blank; param

        for (auto &myVehicle: aParametersRaw.theVehicles) {
            (myVehicle.*field).resize(aParametersRaw.getFrontsCnt());
        }
        for (std::size_t i = 0; i < aParametersRaw.getFrontsCnt(); ++i) {
            REPEAT(2) std::getline(aStream, myLine); // blank; header

            auto myDownloads = parseTwoDimensionMap<T>(
                    aStream, aParametersRaw.theTimeSlotsCount, aParametersRaw.getVehiclesCnt());
            for (std::size_t j = 0; j < aParametersRaw.getVehiclesCnt(); ++j) {
                for (std::size_t k = 0; k < aParametersRaw.theTimeSlotsCount; ++k) {
                    (aParametersRaw.theVehicles[j].*field)[i].push_back(myDownloads[k][j]);
                }
            }
        }
    }
}

ParametersRaw AmplInputParser::parse(std::istream &aStream) const {
    ParametersRaw myParametersRaw{};

    std::string myLine;

    REPEAT(2) std::getline(aStream, myLine); // data; set K
    myParametersRaw.theVehicles.resize(std::stoul(extractLastNumberStr(myLine)));

    REPEAT(2) std::getline(aStream, myLine); // blank; set F
    myParametersRaw.theFronts.resize(std::stoul(extractLastNumberStr(myLine)));

    REPEAT(4) std::getline(aStream, myLine); // blank; set Q; blank; param T
    myParametersRaw.theTimeSlotsCount = std::stoul(extractLastNumberStr(myLine));

    REPEAT(2) std::getline(aStream, myLine); // param V; header
    auto myIsHelicopter = parseTwoDimensionMap<bool>(aStream, 2, myParametersRaw.getVehiclesCnt());
    for (std::size_t i = 0; i < myParametersRaw.getVehiclesCnt(); ++i) {
        myParametersRaw.theVehicles[i].theIsHelicopter = myIsHelicopter[0][i];
    }

    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theFlightDurationLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theRestLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::thePilotPresenceLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theFlightCountLimit);

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param A; header

    auto myAvailability = parseTwoDimensionMap<bool>(
            aStream, myParametersRaw.theTimeSlotsCount, myParametersRaw.getVehiclesCnt());
    for (const auto &myTimeSlot: myAvailability) {
        for (std::size_t i = 0; i < myParametersRaw.getVehiclesCnt(); ++i) {
            myParametersRaw.theVehicles[i].theAvailability.push_back(myTimeSlot[i]);
        }
    }

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param B; header

    auto myIsOnlyHelicopters = parseTwoDimensionMap<bool>(aStream, 2, myParametersRaw.getFrontsCnt());
    for (std::size_t i = 0; i < myParametersRaw.getFrontsCnt(); ++i) {
        myParametersRaw.theFronts[i].theIsOnlyHelicopters = myIsOnlyHelicopters[0][i];
    }

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param U; header

    auto myTransitTime = parseTwoDimensionMap<std::uint32_t>(
            aStream, myParametersRaw.getVehiclesCnt(), myParametersRaw.getFrontsCnt());
    for (std::size_t i = 0; i < myParametersRaw.getVehiclesCnt(); ++i) {
        for (std::size_t j = 0; j < myParametersRaw.getFrontsCnt(); ++j) {
            myParametersRaw.theVehicles[i].theTransitTimes.push_back(myTransitTime[i][j]);
        }
    }

    parseSingleParamAllVehicles<std::uint32_t>(aStream, myParametersRaw, &VehicleRaw::theWaterCapacity);

    REPEAT(3) std::getline(aStream, myLine); // comment; blank; param S
    auto mySimultaneousResourcesLimit = parseOneDimensionMap<std::uint32_t>(aStream, myParametersRaw.getFrontsCnt());
    for (std::size_t i = 0; i < myParametersRaw.getFrontsCnt(); ++i) {
        myParametersRaw.theFronts[i].theSimultaneousResourcesLimit = mySimultaneousResourcesLimit[i];
    }

    parseDownloadsAllVehicles<double>(aStream, myParametersRaw, &VehicleRaw::theIntermediateDownloads);
    parseDownloadsAllVehicles<double>(aStream, myParametersRaw, &VehicleRaw::theTransitDownloads);

    REPEAT(4) std::getline(aStream, myLine); // comment; blank; param W; header

    auto myTargetWaterContent = parseTwoDimensionMap<double>(
            aStream, myParametersRaw.theTimeSlotsCount, myParametersRaw.getFrontsCnt());

    for (std::size_t i = 0; i < myParametersRaw.getFrontsCnt(); ++i) {
        for (std::size_t j = 0; j < myParametersRaw.theTimeSlotsCount; ++j) {
            myParametersRaw.theFronts[i].theTargetWaterContent.push_back(myTargetWaterContent[j][i]);
        }
    }

    REPEAT(5) std::getline(aStream, myLine); // comment; blank; param M; blank; param a1
    myParametersRaw.theA1 = std::stod(extractLastNumberStr(myLine));

    REPEAT(2) std::getline(aStream, myLine); // blank; param a2
    myParametersRaw.theA2 = std::stod(extractLastNumberStr(myLine));

    REPEAT(2) std::getline(aStream, myLine); // blank; param a3
    myParametersRaw.theA3 = std::stod(extractLastNumberStr(myLine));

    if (aStream.fail()) {
        throw std::runtime_error("Failed to correctly read input");
    }

    return myParametersRaw;
}
