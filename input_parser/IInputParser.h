#pragma once

#include "InstanceRaw.h"

#include <fstream>

class IInputParser {
public:
    virtual ~IInputParser() = default;

    virtual InstanceRaw parse(std::istream &aStream) const = 0;
};
