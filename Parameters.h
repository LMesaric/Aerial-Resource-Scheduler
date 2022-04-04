#pragma once

#include <cstdint>
#include <vector>

using DownloadsPerSlot = std::vector<double>;

struct Front {
    DownloadsPerSlot theTargetWaterContent;
    std::uint32_t theSimultaneousResourcesLimit;

    bool theIsOnlyHelicopters;

    auto operator<=>(const Front &) const = default;
};

struct Vehicle {
    std::uint32_t theWaterCapacity;

    std::uint32_t theFlightDurationLimit;
    std::uint32_t theRestLimit;
    std::uint32_t thePilotPresenceLimit;
    std::uint32_t theFlightCountLimit;

    std::vector<char> theAvailability;

    std::vector<std::uint32_t> theTransitTime;
    std::vector<DownloadsPerSlot> theIntermediateDownloads;
    std::vector<DownloadsPerSlot> theTransitDownloads;

    bool theIsHelicopter;

    auto operator<=>(const Vehicle &) const = default;
};

struct Parameters {
    std::uint32_t theTimeSlotsCount;

    std::vector<Front> theFronts;
    std::vector<Vehicle> theVehicles;

    [[nodiscard]] inline std::size_t getFrontsCnt() const noexcept {
        return theFronts.size();
    }

    [[nodiscard]] inline std::size_t getVehiclesCnt() const noexcept {
        return theVehicles.size();
    }

    auto operator<=>(const Parameters &) const = default;
};
