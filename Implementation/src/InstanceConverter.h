#pragma once

#include "Instance.h"
#include "input_parser/InstanceRaw.h"

#include <algorithm>
#include <cstdint>


Front convertFront(const FrontRaw &aFrontRaw) {
    return Front{
            .theTargetWaterContent=aFrontRaw.theTargetWaterContent,
            .theSimultaneousResourcesLimit=aFrontRaw.theSimultaneousResourcesLimit,
            .theIsOnlyHelicopters=aFrontRaw.theIsOnlyHelicopters
    };
}

Vehicle convertVehicle(const VehicleRaw &aVehicleRaw) {
    std::vector<char> myAvailability(aVehicleRaw.theAvailability.begin(), aVehicleRaw.theAvailability.end());

    const auto myCapacity = aVehicleRaw.theWaterCapacity;

    Matrix2<double> myIntermediateDownloads{
            aVehicleRaw.theIntermediateDownloads.size(),
            aVehicleRaw.theIntermediateDownloads[0].size()
    };
    for (std::size_t myFrontId = 0; myFrontId < myIntermediateDownloads.getX(); ++myFrontId) {
        auto myRow = myIntermediateDownloads[myFrontId];
        for (std::size_t mySlot = 0; mySlot < myIntermediateDownloads.getY(); ++mySlot) {
            myRow[mySlot] = myCapacity * aVehicleRaw.theIntermediateDownloads[myFrontId][mySlot];
        }
    }

    Matrix2<double> myTransitDownloads{
            aVehicleRaw.theTransitDownloads.size(),
            aVehicleRaw.theTransitDownloads[0].size()
    };
    for (std::size_t myFrontId = 0; myFrontId < myTransitDownloads.getX(); ++myFrontId) {
        auto myRow = myTransitDownloads[myFrontId];
        for (std::size_t mySlot = 0; mySlot < myTransitDownloads.getY(); ++mySlot) {
            myRow[mySlot] = myCapacity * aVehicleRaw.theTransitDownloads[myFrontId][mySlot];
        }
    }

    return Vehicle{
            .theFlightDurationLimit=aVehicleRaw.theFlightDurationLimit,
            .theRestLimit=aVehicleRaw.theRestLimit,
            .thePilotPresenceLimit=aVehicleRaw.thePilotPresenceLimit,
            .theFlightCountLimit=aVehicleRaw.theFlightCountLimit,
            .theAvailability=myAvailability,
            .theTransitTimes=aVehicleRaw.theTransitTimes,
            .theIntermediateDownloads=std::move(myIntermediateDownloads),
            .theTransitDownloads=std::move(myTransitDownloads),
            .theIsHelicopter=aVehicleRaw.theIsHelicopter
    };
}

Instance convertInstance(const InstanceRaw &anInstanceRaw) {
    std::vector<Vehicle> myVehicles{};
    std::transform(anInstanceRaw.theVehicles.begin(), anInstanceRaw.theVehicles.end(),
                   std::back_inserter(myVehicles),
                   [](const VehicleRaw &aVehicle) { return convertVehicle(aVehicle); });

    std::vector<Front> myFronts{};
    std::transform(anInstanceRaw.theFronts.begin(), anInstanceRaw.theFronts.end(),
                   std::back_inserter(myFronts),
                   [](const FrontRaw &aFront) { return convertFront(aFront); });

    return Instance{
            anInstanceRaw.theTimeSlotsCount,
            std::move(myVehicles),
            std::move(myFronts),
            anInstanceRaw.theA1,
            anInstanceRaw.theA2,
            anInstanceRaw.theA3
    };
}
