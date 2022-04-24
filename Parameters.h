#pragma once

#include "Matrix.h"

#include <array>
#include <vector>


struct Front {
    std::vector<double> theTargetWaterContent;
    std::uint32_t theSimultaneousResourcesLimit;

    bool theIsOnlyHelicopters;
};

struct Vehicle {
    std::uint32_t theFlightDurationLimit;
    std::uint32_t theRestLimit;
    std::uint32_t thePilotPresenceLimit;
    std::uint32_t theFlightCountLimit;

    std::vector<char> theAvailability;

    std::vector<std::uint32_t> theTransitTimes;
    Matrix2<double> theIntermediateDownloads;
    Matrix2<double> theTransitDownloads;

    bool theIsHelicopter;
};

struct ObjectiveFunctionMultipliers {
    double theA1{};
    double theA2{};
    double theA3{};
};

struct Parameters {
    const std::size_t theTimeSlotsCount;

    std::vector<Front> theFronts;
    std::vector<Vehicle> theVehicles;

    std::array<std::vector<std::size_t>, 2> theVehicleIdsByType{};  // 0 == plane; 1 == helicopter

    ObjectiveFunctionMultipliers theObjectiveFunctionMultipliers;

    Parameters(std::size_t aTimeSlotsCount,
               std::vector<Front> aFronts,
               std::vector<Vehicle> aVehicles,
               double anA1, double anA2, double anA3) :
            theTimeSlotsCount{aTimeSlotsCount},
            theFronts{std::move(aFronts)},
            theVehicles{std::move(aVehicles)},
            theObjectiveFunctionMultipliers{.theA1=anA1, .theA2=anA2, .theA3=anA3} {

        for (std::size_t myVehicleId = 0; myVehicleId < getVehiclesCnt(); ++myVehicleId) {
            theVehicleIdsByType[theVehicles[myVehicleId].theIsHelicopter].push_back(myVehicleId);
        }
    }

    [[nodiscard]] inline std::size_t getFrontsCnt() const noexcept {
        return theFronts.size();
    }

    [[nodiscard]] inline std::size_t getVehiclesCnt() const noexcept {
        return theVehicles.size();
    }
};
