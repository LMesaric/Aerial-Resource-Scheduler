#include "Schedule.h"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <utility>


void Schedule::initWaterTarget() {
    for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
        auto myRow = theWaterTargetSurplus[myFrontId];
        const auto &myFrontTargets = theInstance->theFronts[myFrontId].theTargetWaterContent;
        for (std::size_t mySlot = 0; mySlot < theInstance->theTimeSlotsCount; ++mySlot) {
            myRow[mySlot] = -myFrontTargets[mySlot];
        }
    }
}

void Schedule::initRemainingSimultaneousResources() {
    for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
        auto myRow = theRemainingSimultaneousResources[myFrontId];
        const std::uint32_t myLimit = theInstance->theFronts[myFrontId].theSimultaneousResourcesLimit;
        std::fill(myRow.begin(), myRow.end(), myLimit);
    }
}

void Schedule::initRemainingFlights() {
    std::transform(theInstance->theVehicles.begin(), theInstance->theVehicles.end(),
                   std::back_inserter(theRemainingTakeoffsPerVehicle),
                   [](const Vehicle &aVehicle) { return aVehicle.theFlightCountLimit; });
}

void Schedule::assignBasicAvailability() {
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

std::vector<std::size_t> Schedule::unavailabilityBeginnings(const std::vector<char> &anAvailability) const {
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

void Schedule::blockTakeoffsBeforeBasicUnavailability() {
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

void Schedule::blockDistantFronts() {
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

void Schedule::blockPlanesOnHelicopterFronts() {
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

bool Schedule::isRemainingSimultaneousLegal(const Takeoff &aTakeoff) const {
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
std::uint32_t Schedule::findAllLegalTakeoffsInternal(std::vector<Takeoff> *aVector) const {
    assert(((void) "Vector not provided", !isFilling || aVector != nullptr));

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
                    if constexpr (isFilling) {
                        aVector->push_back(myTakeoff);
                    }
                }
            }
        }
    }
    return myTakeoffsCnt;
}

void Schedule::modifyTakeoff(const Takeoff &aTakeoff, ModifyTakeoffMode aMode) {
    const int aFactor = aMode == ModifyTakeoffMode::ADD ? +1 : -1;

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
    auto myTransitDownloadsCountRow = myVehicle.theTransitDownloadsCount[aTakeoff.theFrontId];
    auto myTransitDownloadsVolumeRow = myVehicle.theTransitDownloadsVolume[aTakeoff.theFrontId];
    std::array<std::int32_t, 2> myTransitSlots{myFirstSlotOnFront, myLastSlotOnFront};
    for (std::size_t i = myFirstSlotOnFront == myLastSlotOnFront; i < myTransitSlots.size(); i++) {
        const auto mySlot = myTransitSlots[i];
        theTotalDropsCount += myTransitDownloadsCountRow[mySlot] * aFactor;
        const double myDropVolume = myTransitDownloadsVolumeRow[mySlot] * aFactor;
        myWaterTargetSurplusRow[mySlot] += myDropVolume;
        theTotalWaterOutput += myDropVolume;
    }

    auto myIntermediateDownloadsCountRow = myVehicle.theIntermediateDownloadsCount[aTakeoff.theFrontId];
    auto myIntermediateDownloadsVolumeRow = myVehicle.theIntermediateDownloadsVolume[aTakeoff.theFrontId];
    for (auto mySlot = myFirstSlotOnFront + 1; mySlot < myLastSlotOnFront; ++mySlot) {
        theTotalDropsCount += myIntermediateDownloadsCountRow[mySlot] * aFactor;
        const double myDropVolume = myIntermediateDownloadsVolumeRow[mySlot] * aFactor;
        myWaterTargetSurplusRow[mySlot] += myDropVolume;
        theTotalWaterOutput += myDropVolume;
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

Schedule::Schedule(const Instance *anInstance) :
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

bool Schedule::isLegalTakeoff(const Takeoff &aTakeoff) const {
    return theTakeoffBlockersCount[aTakeoff.theVehicleId][aTakeoff.theFrontId][aTakeoff.theTimeslot] == 0 &&
           theRemainingTakeoffsPerVehicle[aTakeoff.theVehicleId] > 0 &&
           isRemainingSimultaneousLegal(aTakeoff);
}

std::vector<Takeoff> Schedule::findAllLegalTakeoffs() const {
    // myTakeoffs.reserve will cause heap contention for large values.
    std::vector<Takeoff> myTakeoffs{};
    findAllLegalTakeoffsInternal<true>(&myTakeoffs);
    return myTakeoffs;
}

std::uint32_t Schedule::findAllLegalTakeoffsCount() const {
    return findAllLegalTakeoffsInternal<false>();
}

void Schedule::insertTakeoff(const Takeoff &aTakeoff) {
    assert(((void) "Illegal takeoff inserted", isLegalTakeoff(aTakeoff)));
    theTakeoffs.insert(aTakeoff);
    modifyTakeoff(aTakeoff, ModifyTakeoffMode::ADD);
}

void Schedule::removeTakeoff(const Takeoff &aTakeoff) {
    assert(((void) "Nonexistent takeoff removed", theTakeoffs.contains(aTakeoff)));
    theTakeoffs.erase(aTakeoff);
    modifyTakeoff(aTakeoff, ModifyTakeoffMode::REMOVE);
}

Matrix3<std::uint8_t> Schedule::buildTakeoffsMatrix() const {
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

Matrix3<std::uint8_t> Schedule::buildFullScheduleMatrix() const {
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

Matrix2<std::string> Schedule::buildFullScheduleCondensedMatrix() const {
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

std::ostream &operator<<(std::ostream &os, const Schedule &aSchedule) {
    os << "Takeoffs:\n" << aSchedule.buildTakeoffsMatrix()
       << "Water surplus:\n" << aSchedule.getWaterTargetSurplus()
       << "Total water output: " << aSchedule.getTotalWaterOutput()
       << std::endl;
    return os;
}

double Schedule::getMinimumSurplus() const {
    return *std::min_element(theWaterTargetSurplus.getRawData().begin(), theWaterTargetSurplus.getRawData().end());
}

double Schedule::getNegativeSurplusSum() const {
    double myNegativeSum = 0.0;

    for (const double mySurplus: theWaterTargetSurplus.getRawData()) {
        myNegativeSum += std::min(0.0, mySurplus);
    }
    return myNegativeSum;
}

double Schedule::getNegativeSurplusSumWithPriorities() const {
    double myNegativeSumWithPriorities = 0.0;

    for (std::size_t myFrontId = 0; myFrontId < theInstance->getFrontsCnt(); ++myFrontId) {
        for (const double mySurplus: theWaterTargetSurplus[myFrontId]) {
            myNegativeSumWithPriorities += std::min(0.0, mySurplus) * theInstance->theFronts[myFrontId].thePriority;
        }
    }

    return myNegativeSumWithPriorities;
}
