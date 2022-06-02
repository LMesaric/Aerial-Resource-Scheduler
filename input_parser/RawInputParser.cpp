#include "RawInputParser.h"


namespace {
    template<typename T>
    void parseSingleParamAllVehicles(std::istream &aStream, ParametersRaw &aParametersRaw, T VehicleRaw::* field) {
        for (auto &myVehicle: aParametersRaw.theVehicles) {
            aStream >> myVehicle.*field;
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(std::istream &aStream, ParametersRaw &aParametersRaw,
                                   std::vector<std::vector<T>> VehicleRaw::* field) {

        for (auto &myVehicle: aParametersRaw.theVehicles) {
            (myVehicle.*field).resize(aParametersRaw.getFrontsCnt());
        }
        for (std::size_t i = 0; i < aParametersRaw.getFrontsCnt(); ++i) {
            for (std::size_t j = 0; j < aParametersRaw.theTimeSlotsCount; ++j) {
                for (auto &myVehicle: aParametersRaw.theVehicles) {
                    T myDownload;
                    aStream >> myDownload;
                    (myVehicle.*field)[i].push_back(std::move(myDownload));
                }
            }
        }
    }
}

ParametersRaw RawInputParser::parse(std::istream &aStream) const {
    ParametersRaw myParametersRaw{};

    std::size_t myVehiclesCount;
    aStream >> myVehiclesCount;
    myParametersRaw.theVehicles.resize(myVehiclesCount);

    std::size_t myFrontsCount;
    aStream >> myFrontsCount;
    myParametersRaw.theFronts.resize(myFrontsCount);

    aStream >> myParametersRaw.theTimeSlotsCount;

    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theIsHelicopter);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theFlightDurationLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theRestLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::thePilotPresenceLimit);
    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theFlightCountLimit);

    for (std::size_t i = 0; i < myParametersRaw.theTimeSlotsCount; ++i) {
        for (auto &myVehicle: myParametersRaw.theVehicles) {
            bool myAvailability;
            aStream >> myAvailability;
            myVehicle.theAvailability.push_back(myAvailability);
        }
    }

    for (auto &myFront: myParametersRaw.theFronts) {
        aStream >> myFront.theIsOnlyHelicopters;
    }

    for (auto &myVehicle: myParametersRaw.theVehicles) {
        for (std::size_t i = 0; i < myParametersRaw.getFrontsCnt(); ++i) {
            std::uint32_t myTransitTime;
            aStream >> myTransitTime;
            myVehicle.theTransitTimes.push_back(myTransitTime);
        }
    }

    parseSingleParamAllVehicles(aStream, myParametersRaw, &VehicleRaw::theWaterCapacity);

    for (auto &myFront: myParametersRaw.theFronts) {
        aStream >> myFront.theSimultaneousResourcesLimit;
    }

    parseDownloadsAllVehicles(aStream, myParametersRaw, &VehicleRaw::theIntermediateDownloads);
    parseDownloadsAllVehicles(aStream, myParametersRaw, &VehicleRaw::theTransitDownloads);

    for (auto &myFront: myParametersRaw.theFronts) {
        myFront.theTargetWaterContent.resize(myParametersRaw.theTimeSlotsCount);
    }
    for (std::size_t i = 0; i < myParametersRaw.theTimeSlotsCount; ++i) {
        for (auto &myFront: myParametersRaw.theFronts) {
            aStream >> myFront.theTargetWaterContent[i];
        }
    }

    aStream >> myParametersRaw.theA1 >> myParametersRaw.theA2 >> myParametersRaw.theA3;

    if (aStream.fail()) {
        throw std::runtime_error("Failed to correctly read input");
    }

    return myParametersRaw;
}
