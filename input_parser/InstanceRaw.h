#pragma once

#include <vector>


using DownloadsPerSlot = std::vector<double>;

struct FrontRaw {
    DownloadsPerSlot theTargetWaterContent;
    std::uint32_t theSimultaneousResourcesLimit;

    bool theIsOnlyHelicopters;
};

struct VehicleRaw {
    std::uint32_t theWaterCapacity;

    std::uint32_t theFlightDurationLimit;
    std::uint32_t theRestLimit;
    std::uint32_t thePilotPresenceLimit;
    std::uint32_t theFlightCountLimit;

    std::vector<bool> theAvailability;

    std::vector<std::uint32_t> theTransitTimes;
    std::vector<DownloadsPerSlot> theIntermediateDownloads;
    std::vector<DownloadsPerSlot> theTransitDownloads;

    bool theIsHelicopter;
};

struct InstanceRaw {
    std::size_t theTimeSlotsCount;

    std::vector<FrontRaw> theFronts;
    std::vector<VehicleRaw> theVehicles;

    double theA1;
    double theA2;
    double theA3;

    [[nodiscard]] inline std::size_t getFrontsCnt() const noexcept {
        return theFronts.size();
    }

    [[nodiscard]] inline std::size_t getVehiclesCnt() const noexcept {
        return theVehicles.size();
    }
};
