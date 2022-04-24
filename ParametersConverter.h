#pragma once

#include "Parameters.h"
#include "input_parser/ParametersRaw.h"

#include <algorithm>


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
    for (std::size_t myFrontId = 0; myFrontId < myIntermediateDownloads.theX; ++myFrontId) {
        for (std::size_t mySlot = 0; mySlot < myIntermediateDownloads.theY; ++mySlot) {
            myIntermediateDownloads(myFrontId, mySlot) =
                    myCapacity * aVehicleRaw.theIntermediateDownloads[myFrontId][mySlot];
        }
    }

    Matrix2<double> myTransitDownloads{
            aVehicleRaw.theTransitDownloads.size(),
            aVehicleRaw.theTransitDownloads[0].size()
    };
    for (std::size_t myFrontId = 0; myFrontId < myTransitDownloads.theX; ++myFrontId) {
        for (std::size_t mySlot = 0; mySlot < myTransitDownloads.theY; ++mySlot) {
            myTransitDownloads(myFrontId, mySlot) = myCapacity * aVehicleRaw.theTransitDownloads[myFrontId][mySlot];
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

Parameters convertParameters(const ParametersRaw &aParametersRaw) {
    std::vector<Front> myFronts{};
    std::transform(aParametersRaw.theFronts.begin(), aParametersRaw.theFronts.end(),
                   std::back_inserter(myFronts),
                   [](const FrontRaw &aFront) { return convertFront(aFront); });

    std::vector<Vehicle> myVehicles{};
    std::transform(aParametersRaw.theVehicles.begin(), aParametersRaw.theVehicles.end(),
                   std::back_inserter(myVehicles),
                   [](const VehicleRaw &aVehicle) { return convertVehicle(aVehicle); });

    return Parameters{
            aParametersRaw.theTimeSlotsCount,
            std::move(myFronts),
            std::move(myVehicles),
            aParametersRaw.theA1,
            aParametersRaw.theA2,
            aParametersRaw.theA3
    };
}
