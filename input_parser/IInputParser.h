#pragma once

#include "ParametersRaw.h"

#include <fstream>

class IInputParser {
public:
    virtual ~IInputParser() = default;

    virtual ParametersRaw parse(std::istream &aStream) const = 0;
};
