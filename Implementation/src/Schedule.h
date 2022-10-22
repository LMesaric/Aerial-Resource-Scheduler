#pragma once

#include "Instance.h"
#include "Matrix.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <unordered_set>
#include <utility>
#include <vector>


struct Takeoff {
    std::uint8_t theVehicleId{};
    std::uint8_t theFrontId{};
    std::uint8_t theTimeslot{};

    Takeoff(std::uint8_t aVehicleId, std::uint8_t aFrontId, std::uint8_t aTimeslot) :
            theVehicleId{aVehicleId},
            theFrontId{aFrontId},
            theTimeslot{aTimeslot} {}

    bool operator<(const Takeoff &rhs) const {
        return std::tie(theVehicleId, theFrontId, theTimeslot) <
               std::tie(rhs.theVehicleId, rhs.theFrontId, rhs.theTimeslot);
    }

    bool operator==(const Takeoff &rhs) const {
        return std::tie(theVehicleId, theFrontId, theTimeslot) ==
               std::tie(rhs.theVehicleId, rhs.theFrontId, rhs.theTimeslot);
    }

    bool operator!=(const Takeoff &rhs) const {
        return !(rhs == *this);
    }

    friend std::ostream &operator<<(std::ostream &os, const Takeoff &aTakeoff) {
        os << "theVehicleId: " << aTakeoff.theVehicleId <<
           " theFrontId: " << aTakeoff.theFrontId <<
           " theTimeslot: " << aTakeoff.theTimeslot;
        return os;
    }
};

namespace std {
    template<>
    struct hash<Takeoff> {
        std::size_t operator()(const Takeoff &t) const {
            return hash<std::uint32_t>()(t.theVehicleId)
                   ^ hash<std::uint32_t>()(t.theFrontId)
                   ^ hash<std::uint32_t>()(t.theTimeslot);
        }
    };
}


class Schedule {
    static constexpr std::uint8_t IMPOSSIBLE_BLOCKERS_COUNT{2};  // Any value above 0 would work fine.

    const Instance *theInstance{};

    std::unordered_set<Takeoff> theTakeoffs{};

    Matrix3<std::uint8_t> theTakeoffBlockersCount{};

    // Negative if target is not met, positive if there is surplus.
    Matrix2<double> theWaterTargetSurplus{};

    double theTotalWaterOutput{0.0};

    Matrix2<std::uint32_t> theRemainingSimultaneousResources{};

    std::vector<std::uint32_t> theRemainingTakeoffsPerVehicle{};

    void initWaterTarget() {
        for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
            auto myRow = theWaterTargetSurplus[myFrontId];
            const auto &myFrontTargets = theInstance->theFronts[myFrontId].theTargetWaterContent;
            for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                myRow[mySlot] = -myFrontTargets[mySlot];
            }
        }
    }

    void initRemainingSimultaneousResources() {
        for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
            auto myRow = theRemainingSimultaneousResources[myFrontId];
            const std::uint32_t myLimit = theInstance->theFronts[myFrontId].theSimultaneousResourcesLimit;
            std::fill(myRow.begin(), myRow.end(), myLimit);
        }
    }

    void initRemainingFlights() {
        std::transform(theInstance->theVehicles.begin(), theInstance->theVehicles.end(),
                       std::back_inserter(theRemainingTakeoffsPerVehicle),
                       [](const Vehicle &aVehicle) { return aVehicle.theFlightCountLimit; });
    }

    void assignBasicAvailability() {
        for (std::size_t myVehicleId = 0; myVehicleId < theInstance->getVehiclesCnt(); ++myVehicleId) {
            const auto &myVehicle = theInstance->theVehicles[myVehicleId];
            auto myProxy2 = theTakeoffBlockersCount[myVehicleId];

            for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                const auto myBlockersCount = myVehicle.theAvailability[mySlot] ? 0u : IMPOSSIBLE_BLOCKERS_COUNT;

                for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
                    myProxy2[myFrontId][mySlot] += myBlockersCount;
                }
            }
        }
    }

    [[nodiscard]] std::vector<std::size_t> unavailabilityBeginnings(const std::vector<char> &anAvailability) const {
        std::vector<std::size_t> myBeginnings{};

        char myPreviousSlot = 1;
        for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
            if (anAvailability[mySlot] == 0 && myPreviousSlot != 0) {
                myBeginnings.push_back(mySlot);
            }
            myPreviousSlot = anAvailability[mySlot];
        }

        if (anAvailability[theInstance->theTimeSlotsCount - 1] != 0) {
            myBeginnings.push_back(theInstance->theTimeSlotsCount);
        }

        return myBeginnings;
    }

    void blockTakeoffsBeforeBasicUnavailability() {
        // Assuming unavailability period includes resting period.

        for (std::size_t myVehicleId = 0; myVehicleId < theInstance->getVehiclesCnt(); ++myVehicleId) {
            const auto &myVehicle = theInstance->theVehicles[myVehicleId];
            auto myProxy2 = theTakeoffBlockersCount[myVehicleId];

            for (const std::size_t myBeginning: unavailabilityBeginnings(myVehicle.theAvailability)) {
                const std::size_t myFirstUnavailableTakeoffSlot = myBeginning >= myVehicle.theFlightDurationLimit - 1
                                                                  ? myBeginning - (myVehicle.theFlightDurationLimit - 1)
                                                                  : 0;

                for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
                    auto myRow = myProxy2[myFrontId];

                    for (std::size_t mySlot = myFirstUnavailableTakeoffSlot; mySlot < myBeginning; ++mySlot) {
                        myRow[mySlot] += IMPOSSIBLE_BLOCKERS_COUNT;
                    }
                }
            }
        }
    }

    void blockDistantFronts() {
        // Block all takeoffs for which the transit time is so long that the resource would have no time left
        // to make any drops.

        for (std::size_t myVehicleId = 0; myVehicleId < theInstance->getVehiclesCnt(); ++myVehicleId) {
            const auto &myVehicle = theInstance->theVehicles[myVehicleId];
            auto myProxy2 = theTakeoffBlockersCount[myVehicleId];

            for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
                if (2 * myVehicle.theTransitTimes[myFrontId] >= myVehicle.theFlightDurationLimit) {
                    auto myRow = myProxy2[myFrontId];

                    for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                        myRow[mySlot] += IMPOSSIBLE_BLOCKERS_COUNT;
                    }
                }
            }
        }
    }

    void blockPlanesOnHelicopterFronts() {
        for (const auto myPlaneId: theInstance->theVehicleIdsByType[0]) {
            auto myProxy2 = theTakeoffBlockersCount[myPlaneId];

            for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
                if (!theInstance->theFronts[myFrontId].theIsOnlyHelicopters) {
                    continue;
                }

                auto myRow = myProxy2[myFrontId];
                for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                    myRow[mySlot] += IMPOSSIBLE_BLOCKERS_COUNT;
                }
            }
        }
    }

    [[nodiscard]] bool isRemainingSimultaneousLegal(const Takeoff &aTakeoff) const {
        const auto &myVehicle = theInstance->theVehicles[aTakeoff.theVehicleId];

        const auto myTransitTime = static_cast<std::int32_t>(myVehicle.theTransitTimes[aTakeoff.theFrontId]);
        const auto myRestStartSlot = static_cast<std::int32_t>(aTakeoff.theTimeslot + myVehicle.theFlightDurationLimit);

        const auto myFirstSlotOnFront = static_cast<std::int32_t>(aTakeoff.theTimeslot) + myTransitTime;
        const auto myLastSlotOnFront = myRestStartSlot - myTransitTime - 1;

        auto mySimultaneousRow = theRemainingSimultaneousResources[aTakeoff.theFrontId];
        for (auto mySlot = myFirstSlotOnFront; mySlot <= myLastSlotOnFront; ++mySlot) {
            if (mySimultaneousRow[mySlot] == 0) {
                return false;
            }
        }
        return true;
    }

    template<bool isFilling>
    std::uint32_t findAllLegalTakeoffsInternal(std::vector<Takeoff> *aVector = nullptr) const {
        std::uint32_t myTakeoffsCnt = 0;

        for (std::size_t myVehicleId = 0; myVehicleId < theInstance->getVehiclesCnt(); ++myVehicleId) {
            if (theRemainingTakeoffsPerVehicle[myVehicleId] == 0) {
                continue;
            }

            auto theTakeoffBlockersCountProxy = theTakeoffBlockersCount[myVehicleId];

            for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
                auto theTakeoffBlockersCountRow = theTakeoffBlockersCountProxy[myFrontId];

                for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                    Takeoff myTakeoff(myVehicleId, myFrontId, mySlot);

                    if (theTakeoffBlockersCountRow[mySlot] == 0 && isRemainingSimultaneousLegal(myTakeoff)) {
                        myTakeoffsCnt++;
                        if constexpr(isFilling) {
                            aVector->push_back(myTakeoff);
                        }
                    }
                }
            }
        }
        return myTakeoffsCnt;
    }

    template<int aFactor>
    void modifyTakeoff(const Takeoff &aTakeoff) {
        static_assert(aFactor == 1 || aFactor == -1, "Unexpected factor");

        const auto &myVehicle = theInstance->theVehicles[aTakeoff.theVehicleId];

        theRemainingTakeoffsPerVehicle[aTakeoff.theVehicleId] -= aFactor;

        const auto myTransitTime = static_cast<std::int32_t>(myVehicle.theTransitTimes[aTakeoff.theFrontId]);
        const auto myRestStartSlot = static_cast<std::int32_t>(aTakeoff.theTimeslot + myVehicle.theFlightDurationLimit);

        const auto myFirstSlotOnFront = static_cast<std::int32_t>(aTakeoff.theTimeslot) + myTransitTime;
        const auto myLastSlotOnFront = myRestStartSlot - myTransitTime - 1;

        auto mySimultaneousRow = theRemainingSimultaneousResources[aTakeoff.theFrontId];
        for (auto mySlot = myFirstSlotOnFront; mySlot <= myLastSlotOnFront; ++mySlot) {
            mySimultaneousRow[mySlot] -= aFactor;
        }

        auto myWaterTargetSurplusRow = theWaterTargetSurplus[aTakeoff.theFrontId];

        // In case when myFirstSlotOnFront == myLastSlotOnFront, the amount of water should be even lower
        // than this due to a slot being in two-way transit.
        auto myTransitDownloadsRow = myVehicle.theTransitDownloads[aTakeoff.theFrontId];
        std::array<std::int32_t, 2> myTransitSlots{myFirstSlotOnFront, myLastSlotOnFront};
        for (std::size_t i = myFirstSlotOnFront == myLastSlotOnFront; i < myTransitSlots.size(); i++) {
            const auto mySlot = myTransitSlots[i];
            const double myDrop = myTransitDownloadsRow[mySlot] * aFactor;
            myWaterTargetSurplusRow[mySlot] += myDrop;
            theTotalWaterOutput += myDrop;
        }

        auto myIntermediateDownloadsRow = myVehicle.theIntermediateDownloads[aTakeoff.theFrontId];
        for (auto mySlot = myFirstSlotOnFront + 1; mySlot < myLastSlotOnFront; ++mySlot) {
            const double myDrop = myIntermediateDownloadsRow[mySlot] * aFactor;
            myWaterTargetSurplusRow[mySlot] += myDrop;
            theTotalWaterOutput += myDrop;
        }

        const std::size_t myFlightAndRestDuration = myVehicle.theFlightDurationLimit + myVehicle.theRestLimit;

        const std::int32_t myPreviousLatestPossibleTakeoff =
                aTakeoff.theTimeslot >= myFlightAndRestDuration
                ? static_cast<std::int32_t>(aTakeoff.theTimeslot - myFlightAndRestDuration)
                : -1;

        const auto myNextEarliestPossibleTakeoff =
                std::min(aTakeoff.theTimeslot + myFlightAndRestDuration, theInstance->theTimeSlotsCount);

        const auto myEarliestPossibleTakeoff =
                myRestStartSlot >= myVehicle.thePilotPresenceLimit
                ? myRestStartSlot - myVehicle.thePilotPresenceLimit
                : 0;

        const auto myLatestPossibleTakeoff =
                aTakeoff.theTimeslot + myVehicle.thePilotPresenceLimit - myVehicle.theFlightDurationLimit;

        auto myBlockersProxy2 = theTakeoffBlockersCount[aTakeoff.theVehicleId];
        for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
            auto myBlockersRow = myBlockersProxy2[myFrontId];
            for (auto mySlot = 0; mySlot < myEarliestPossibleTakeoff; ++mySlot) {
                myBlockersRow[mySlot] += aFactor;
            }
            for (auto mySlot = myPreviousLatestPossibleTakeoff + 1; mySlot < myNextEarliestPossibleTakeoff; ++mySlot) {
                myBlockersRow[mySlot] += aFactor;
            }
            for (auto mySlot = myLatestPossibleTakeoff + 1; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
                myBlockersRow[mySlot] += aFactor;
            }
        }

        for (const auto myOtherVehicleId: theInstance->theVehicleIdsByType[!myVehicle.theIsHelicopter]) {
            const auto &myOtherVehicle = theInstance->theVehicles[myOtherVehicleId];

            const auto myOtherTransitTime = static_cast<std::int32_t>(
                    myOtherVehicle.theTransitTimes[aTakeoff.theFrontId]
            );
            std::int32_t myOtherFirstBlockedTimeslot =
                    myFirstSlotOnFront + myOtherTransitTime + 1
                    - static_cast<std::int32_t>(myOtherVehicle.theFlightDurationLimit);
            myOtherFirstBlockedTimeslot = std::max(0, myOtherFirstBlockedTimeslot);

            const std::int32_t myOtherFirstNextAvailableTimeslot = myLastSlotOnFront - myOtherTransitTime + 1;

            auto myBlockersRow = theTakeoffBlockersCount[myOtherVehicleId][aTakeoff.theFrontId];
            for (auto mySlot = myOtherFirstBlockedTimeslot; mySlot < myOtherFirstNextAvailableTimeslot; ++mySlot) {
                myBlockersRow[mySlot] += aFactor;
            }
        }
    }

public:
    Schedule() = default;

    explicit Schedule(const Instance *anInstance) :
            theInstance{anInstance},
            theTakeoffBlockersCount{
                    theInstance->getVehiclesCnt(),
                    theInstance->getFrontsCnt(),
                    theInstance->theTimeSlotsCount
            },
            theWaterTargetSurplus{
                    theInstance->getFrontsCnt(),
                    theInstance->theTimeSlotsCount
            },
            theRemainingSimultaneousResources{
                    theInstance->getFrontsCnt(),
                    theInstance->theTimeSlotsCount
            } {

        initWaterTarget();
        initRemainingSimultaneousResources();
        initRemainingFlights();

        assignBasicAvailability();
        blockTakeoffsBeforeBasicUnavailability();
        blockDistantFronts();
        blockPlanesOnHelicopterFronts();
    }

    [[nodiscard]] bool isLegalTakeoff(const Takeoff &aTakeoff) const {
        return theTakeoffBlockersCount[aTakeoff.theVehicleId][aTakeoff.theFrontId][aTakeoff.theTimeslot] == 0 &&
               theRemainingTakeoffsPerVehicle[aTakeoff.theVehicleId] > 0 &&
               isRemainingSimultaneousLegal(aTakeoff);
    }

    [[nodiscard]] std::vector<Takeoff> findAllLegalTakeoffs() const {
        // myTakeoffs.reserve will cause heap contention for large values.
        std::vector<Takeoff> myTakeoffs{};
        findAllLegalTakeoffsInternal<true>(&myTakeoffs);
        return myTakeoffs;
    }

    [[nodiscard]] inline std::uint32_t findAllLegalTakeoffsCount() const {
        return findAllLegalTakeoffsInternal<false>();
    }

    void insertTakeoff(const Takeoff &aTakeoff) {
        assert(((void) "Illegal takeoff inserted", isLegalTakeoff(aTakeoff)));
        theTakeoffs.insert(aTakeoff);
        modifyTakeoff<+1>(aTakeoff);
    }

    void removeTakeoff(const Takeoff &aTakeoff) {
        assert(((void) "Nonexistent takeoff removed", theTakeoffs.contains(aTakeoff)));
        theTakeoffs.erase(aTakeoff);
        modifyTakeoff<-1>(aTakeoff);
    }

    [[nodiscard]] Matrix3<std::uint8_t> buildTakeoffsMatrix() const {
        Matrix3<std::uint8_t> myTakeoffsMatrix{
                theInstance->getVehiclesCnt(),
                theInstance->getFrontsCnt(),
                theInstance->theTimeSlotsCount
        };

        for (const auto &myTakeoff: theTakeoffs) {
            myTakeoffsMatrix[myTakeoff.theVehicleId][myTakeoff.theFrontId][myTakeoff.theTimeslot] = true;
        }

        return myTakeoffsMatrix;
    }

    [[nodiscard]] Matrix3<std::uint8_t> buildFullScheduleMatrix() const {
        Matrix3<std::uint8_t> myMatrix{
                theInstance->getVehiclesCnt(),
                theInstance->getFrontsCnt(),
                theInstance->theTimeSlotsCount
        };

        for (const auto &myTakeoff: theTakeoffs) {
            const auto &myVehicle = theInstance->theVehicles[myTakeoff.theVehicleId];
            auto myRow = myMatrix[myTakeoff.theVehicleId][myTakeoff.theFrontId];
            for (std::size_t i = 0; i < myVehicle.theFlightDurationLimit; ++i) {
                myRow[myTakeoff.theTimeslot + i] = true;
            }
        }

        return myMatrix;
    }

    [[nodiscard]] Matrix2<std::string> buildFullScheduleCondensedMatrix() const {
        Matrix2<std::string> myMatrix{
                theInstance->getVehiclesCnt(),
                theInstance->theTimeSlotsCount
        };
        myMatrix.fill("-");

        for (const auto &myTakeoff: theTakeoffs) {
            const auto &myVehicle = theInstance->theVehicles[myTakeoff.theVehicleId];
            auto myRow = myMatrix[myTakeoff.theVehicleId];
            for (std::size_t i = 0; i < myVehicle.theFlightDurationLimit; ++i) {
                myRow[myTakeoff.theTimeslot + i] = std::to_string(myTakeoff.theFrontId);
            }
        }

        return myMatrix;
    }

    friend std::ostream &operator<<(std::ostream &os, const Schedule &aSchedule) {
        os << "Takeoffs:\n" << aSchedule.buildTakeoffsMatrix()
           << "Water surplus:\n" << aSchedule.theWaterTargetSurplus
           << "Total water output: " << aSchedule.theTotalWaterOutput
           << std::endl;
        return os;
    }

    [[nodiscard]] double getMinimumSurplus() const {
        return *std::min_element(theWaterTargetSurplus.getRawData().begin(), theWaterTargetSurplus.getRawData().end());
    };

    [[nodiscard]] double getNegativeSurplusSum() const {
        double myNegativeSum = 0.0;

        for (const double mySurplus: theWaterTargetSurplus.getRawData()) {
            myNegativeSum += std::min(0.0, mySurplus);
        }
        return myNegativeSum;
    };


    [[nodiscard]] const Instance &getInstance() const {
        return *theInstance;
    }

    [[nodiscard]] const auto &getTakeoffs() const {
        return theTakeoffs;
    }

    [[nodiscard]] auto getTakeoffsCount() const {
        return theTakeoffs.size();
    }

    [[nodiscard]] const auto &getRemainingSimultaneousResources() const {
        return theRemainingSimultaneousResources;
    }

    [[nodiscard]] const auto &getRemainingTakeoffsPerVehicle() const {
        return theRemainingTakeoffsPerVehicle;
    }

    [[nodiscard]] const auto &getTakeoffBlockersCount() const {
        return theTakeoffBlockersCount;
    }

    [[nodiscard]] const auto &getWaterTargetSurplus() const {
        return theWaterTargetSurplus;
    }

    [[nodiscard]] auto getTotalWaterOutput() const {
        return theTotalWaterOutput;
    }
};
