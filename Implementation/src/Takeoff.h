#pragma once

#include <cstdint>
#include <iostream>
#include <tuple>


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
