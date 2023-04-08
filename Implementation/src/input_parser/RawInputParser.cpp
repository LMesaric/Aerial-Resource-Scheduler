#include "RawInputParser.h"

#include <cstdint>
#include <iostream>


namespace {
    template<typename T>
    void parseSingleParamAllVehicles(std::istream &aStream, InstanceRaw &anInstanceRaw, T VehicleRaw::* field) {
        for (auto &myVehicle: anInstanceRaw.theVehicles) {
            aStream >> myVehicle.*field;
        }
    }

    template<typename T>
    void parseDownloadsAllVehicles(
            std::istream &aStream,
            InstanceRaw &anInstanceRaw,
            std::vector<std::vector<T>> VehicleRaw::* field
    ) {
        for (auto &myVehicle: anInstanceRaw.theVehicles) {
            (myVehicle.*field).resize(anInstanceRaw.getFrontsCnt());
        }
        for (std::size_t i = 0; i < anInstanceRaw.getFrontsCnt(); ++i) {
            for (std::size_t j = 0; j < anInstanceRaw.theTimeSlotsCount; ++j) {
                for (auto &myVehicle: anInstanceRaw.theVehicles) {
                    T myDownload;
                    aStream >> myDownload;
                    (myVehicle.*field)[i].push_back(std::move(myDownload));
                }
            }
        }
    }
}

InstanceRaw RawInputParser::parse(std::istream &aStream) const {
    InstanceRaw myInstanceRaw{};

    std::size_t myVehiclesCount;
    aStream >> myVehiclesCount;
    myInstanceRaw.theVehicles.resize(myVehiclesCount);

    std::size_t myFrontsCount;
    aStream >> myFrontsCount;
    myInstanceRaw.theFronts.resize(myFrontsCount);

    aStream >> myInstanceRaw.theTimeSlotsCount;

    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theIsHelicopter);
    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theFlightDurationLimit);
    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theRestLimit);
    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::thePilotPresenceLimit);
    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theFlightCountLimit);

    for (std::size_t i = 0; i < myInstanceRaw.theTimeSlotsCount; ++i) {
        for (auto &myVehicle: myInstanceRaw.theVehicles) {
            bool myAvailability;
            aStream >> myAvailability;
            myVehicle.theAvailability.push_back(myAvailability);
        }
    }

    for (auto &myFront: myInstanceRaw.theFronts) {
        aStream >> myFront.theIsOnlyHelicopters;
    }

    for (auto &myVehicle: myInstanceRaw.theVehicles) {
        for (std::size_t i = 0; i < myInstanceRaw.getFrontsCnt(); ++i) {
            std::uint32_t myTransitTime;
            aStream >> myTransitTime;
            myVehicle.theTransitTimes.push_back(myTransitTime);
        }
    }

    parseSingleParamAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theWaterCapacity);

    for (auto &myFront: myInstanceRaw.theFronts) {
        aStream >> myFront.theSimultaneousResourcesLimit;
    }

    parseDownloadsAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theIntermediateDownloadsCount);
    parseDownloadsAllVehicles(aStream, myInstanceRaw, &VehicleRaw::theTransitDownloadsCount);

    for (auto &myFront: myInstanceRaw.theFronts) {
        myFront.theTargetWaterContent.resize(myInstanceRaw.theTimeSlotsCount);
    }
    for (std::size_t i = 0; i < myInstanceRaw.theTimeSlotsCount; ++i) {
        for (auto &myFront: myInstanceRaw.theFronts) {
            aStream >> myFront.theTargetWaterContent[i];
        }
    }

    aStream >> myInstanceRaw.theA1 >> myInstanceRaw.theA2 >> myInstanceRaw.theA3;

    for (auto &myFront: myInstanceRaw.theFronts) {
        aStream >> myFront.thePriority;
    }

    if (aStream.fail()) {
        const auto myErrMsg = "Failed to correctly read input";
        std::cout << "FATAL: " << myErrMsg << std::endl;
        throw std::runtime_error(myErrMsg);
    }

    return myInstanceRaw;
}
