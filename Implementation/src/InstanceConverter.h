#pragma once

#include "Instance.h"
#include "input_parser/InstanceRaw.h"

#include <algorithm>
#include <cstdint>


Front convertFront(const FrontRaw &aFrontRaw) {
    return Front{
            .theTargetWaterContent=aFrontRaw.theTargetWaterContent,
            .thePriority=aFrontRaw.thePriority,
            .theSimultaneousResourcesLimit=aFrontRaw.theSimultaneousResourcesLimit,
            .theIsOnlyHelicopters=aFrontRaw.theIsOnlyHelicopters
    };
}

Vehicle convertVehicle(const VehicleRaw &aVehicleRaw) {
    std::vector<char> myAvailability(aVehicleRaw.theAvailability.begin(), aVehicleRaw.theAvailability.end());

    const auto myCapacity = aVehicleRaw.theWaterCapacity;

    Matrix2<double> myIntermediateDownloadsCount{
            aVehicleRaw.theIntermediateDownloadsCount.size(),
            aVehicleRaw.theIntermediateDownloadsCount[0].size()
    };
    Matrix2<double> myIntermediateDownloadsVolume{
            aVehicleRaw.theIntermediateDownloadsCount.size(),
            aVehicleRaw.theIntermediateDownloadsCount[0].size()
    };
    for (std::size_t myFrontId = 0; myFrontId < myIntermediateDownloadsVolume.getX(); ++myFrontId) {
        for (std::size_t mySlot = 0; mySlot < myIntermediateDownloadsVolume.getY(); ++mySlot) {
            auto myCount = aVehicleRaw.theIntermediateDownloadsCount[myFrontId][mySlot];
            myIntermediateDownloadsCount[myFrontId][mySlot] = myCount;
            myIntermediateDownloadsVolume[myFrontId][mySlot] = myCapacity * myCount;
        }
    }

    Matrix2<double> myTransitDownloadsCount{
            aVehicleRaw.theTransitDownloadsCount.size(),
            aVehicleRaw.theTransitDownloadsCount[0].size()
    };
    Matrix2<double> myTransitDownloadsVolume{
            aVehicleRaw.theTransitDownloadsCount.size(),
            aVehicleRaw.theTransitDownloadsCount[0].size()
    };
    for (std::size_t myFrontId = 0; myFrontId < myTransitDownloadsVolume.getX(); ++myFrontId) {
        for (std::size_t mySlot = 0; mySlot < myTransitDownloadsVolume.getY(); ++mySlot) {
            auto myCount = aVehicleRaw.theTransitDownloadsCount[myFrontId][mySlot];
            myTransitDownloadsCount[myFrontId][mySlot] = myCount;
            myTransitDownloadsVolume[myFrontId][mySlot] = myCapacity * myCount;
        }
    }

    return Vehicle{
            .theFlightDurationLimit=aVehicleRaw.theFlightDurationLimit,
            .theRestLimit=aVehicleRaw.theRestLimit,
            .thePilotPresenceLimit=aVehicleRaw.thePilotPresenceLimit,
            .theFlightCountLimit=aVehicleRaw.theFlightCountLimit,
            .theAvailability=myAvailability,
            .theTransitTimes=aVehicleRaw.theTransitTimes,
            .theIntermediateDownloadsCount=std::move(myIntermediateDownloadsCount),
            .theTransitDownloadsCount=std::move(myTransitDownloadsCount),
            .theIntermediateDownloadsVolume=std::move(myIntermediateDownloadsVolume),
            .theTransitDownloadsVolume=std::move(myTransitDownloadsVolume),
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
