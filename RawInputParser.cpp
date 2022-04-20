#include "RawInputParser.h"


namespace {
    template<typename T>
    void parseSingleParamAllVehicles(std::istream &aStream, Parameters &aParameters, T Vehicle::* field) {
        for (auto &myVehicle: aParameters.theVehicles) {
            aStream >> myVehicle.*field;
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(std::istream &aStream, Parameters &aParameters,
                                   std::vector<std::vector<T>> Vehicle::* field) {

        for (auto &myVehicle: aParameters.theVehicles) {
            (myVehicle.*field).resize(aParameters.getFrontsCnt());
        }
        for (int i = 0; i < aParameters.getFrontsCnt(); ++i) {
            for (int j = 0; j < aParameters.theTimeSlotsCount; ++j) {
                for (auto &myVehicle: aParameters.theVehicles) {
                    T myDownload;
                    aStream >> myDownload;
                    (myVehicle.*field)[i].push_back(myDownload);
                }
            }
        }
    }
}

Parameters RawInputParser::parse(std::istream &aStream) const {
    Parameters myParameters{};

    std::size_t myVehiclesCount;
    aStream >> myVehiclesCount;
    myParameters.theVehicles.resize(myVehiclesCount);

    std::size_t myFrontsCount;
    aStream >> myFrontsCount;
    myParameters.theFronts.resize(myFrontsCount);

    aStream >> myParameters.theTimeSlotsCount;

    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::theIsHelicopter);
    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::theFlightDurationLimit);
    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::theRestLimit);
    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::thePilotPresenceLimit);
    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::theFlightCountLimit);

    for (int i = 0; i < myParameters.theTimeSlotsCount; ++i) {
        for (auto &myVehicle: myParameters.theVehicles) {
            bool myAvailability;
            aStream >> myAvailability;
            myVehicle.theAvailability.push_back(static_cast<char>(myAvailability));
        }
    }

    for (auto &myFront: myParameters.theFronts) {
        aStream >> myFront.theIsOnlyHelicopters;
    }

    for (auto &myVehicle: myParameters.theVehicles) {
        for (int i = 0; i < myParameters.getFrontsCnt(); ++i) {
            std::uint32_t myTransitTime;
            aStream >> myTransitTime;
            myVehicle.theTransitTime.push_back(myTransitTime);
        }
    }

    parseSingleParamAllVehicles(aStream, myParameters, &Vehicle::theWaterCapacity);

    for (auto &myFront: myParameters.theFronts) {
        aStream >> myFront.theSimultaneousResourcesLimit;
    }

    parseDownloadsAllVehicles(aStream, myParameters, &Vehicle::theIntermediateDownloads);
    parseDownloadsAllVehicles(aStream, myParameters, &Vehicle::theTransitDownloads);

    for (auto &myFront: myParameters.theFronts) {
        myFront.theTargetWaterContent.resize(myParameters.theTimeSlotsCount);
    }
    for (int i = 0; i < myParameters.theTimeSlotsCount; ++i) {
        for (auto &myFront: myParameters.theFronts) {
            aStream >> myFront.theTargetWaterContent[i];
        }
    }

    int64_t myDump;
    aStream >> myDump >> myParameters.theA1 >> myParameters.theA2 >> myParameters.theA3;

    if (aStream.fail()) {
        throw std::runtime_error("Failed to correctly read input");
    }

    return myParameters;
}
