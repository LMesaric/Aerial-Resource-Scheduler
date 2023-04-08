#pragma once

#include "Instance.h"
#include "Matrix.h"
#include "Takeoff.h"

#include <cstdint>
#include <iostream>
#include <tuple>
#include <unordered_set>
#include <vector>


class Schedule {
    enum class ModifyTakeoffMode {
        ADD, REMOVE
    };
    static constexpr std::uint8_t IMPOSSIBLE_BLOCKERS_COUNT{2};  // Any value above 0 would work fine.

    const Instance *theInstance{};
    std::unordered_set<Takeoff> theTakeoffs{};
    Matrix3<std::uint8_t> theTakeoffBlockersCount{};
    Matrix2<double> theWaterTargetSurplus{};  // Negative if target is not met, positive if there is surplus.
    double theTotalWaterOutput{0.0};
    double theTotalDropsCount{0.0};
    Matrix2<std::uint32_t> theRemainingSimultaneousResources{};
    std::vector<std::uint32_t> theRemainingTakeoffsPerVehicle{};

public:
    Schedule() = default;

    explicit Schedule(const Instance *anInstance);

    [[nodiscard]] bool isLegalTakeoff(const Takeoff &aTakeoff) const;

    [[nodiscard]] std::vector<Takeoff> findAllLegalTakeoffs() const;

    [[nodiscard]] std::uint32_t findAllLegalTakeoffsCount() const;

    void insertTakeoff(const Takeoff &aTakeoff);

    void removeTakeoff(const Takeoff &aTakeoff);

    [[nodiscard]] Matrix3<std::uint8_t> buildTakeoffsMatrix() const;

    [[nodiscard]] Matrix3<std::uint8_t> buildFullScheduleMatrix() const;

    [[nodiscard]] Matrix2<std::string> buildFullScheduleCondensedMatrix() const;

    [[nodiscard]] double getMinimumSurplus() const;

    [[nodiscard]] double getNegativeSurplusSum() const;

    [[nodiscard]] inline const Instance &getInstance() const {
        return *theInstance;
    }

    [[nodiscard]] inline const auto &getTakeoffs() const {
        return theTakeoffs;
    }

    [[nodiscard]] inline auto getTakeoffsCount() const {
        return theTakeoffs.size();
    }

    [[nodiscard]] inline const auto &getRemainingSimultaneousResources() const {
        return theRemainingSimultaneousResources;
    }

    [[nodiscard]] inline const auto &getRemainingTakeoffsPerVehicle() const {
        return theRemainingTakeoffsPerVehicle;
    }

    [[nodiscard]] inline const auto &getTakeoffBlockersCount() const {
        return theTakeoffBlockersCount;
    }

    [[nodiscard]] inline const auto &getWaterTargetSurplus() const {
        return theWaterTargetSurplus;
    }

    [[nodiscard]] inline auto getTotalWaterOutput() const {
        return theTotalWaterOutput;
    }

    [[nodiscard]] inline auto getTotalDropsCount() const {
        return theTotalDropsCount;
    }

    friend std::ostream &operator<<(std::ostream &os, const Schedule &aSchedule);

private:
    void initWaterTarget();

    void initRemainingSimultaneousResources();

    void initRemainingFlights();

    void assignBasicAvailability();

    [[nodiscard]] std::vector<std::size_t> unavailabilityBeginnings(const std::vector<char> &anAvailability) const;

    void blockTakeoffsBeforeBasicUnavailability();

    void blockDistantFronts();

    void blockPlanesOnHelicopterFronts();

    [[nodiscard]] bool isRemainingSimultaneousLegal(const Takeoff &aTakeoff) const;

    template<bool isFilling>
    std::uint32_t findAllLegalTakeoffsInternal(std::vector<Takeoff> *aVector = nullptr) const;

    void modifyTakeoff(const Takeoff &aTakeoff, ModifyTakeoffMode aMode);
};
