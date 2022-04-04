#pragma once

#include "Parameters.h"

#include <fstream>

class IInputParser {
public:
    virtual ~IInputParser() = default;

    virtual Parameters parse(std::ifstream &aStream) = 0;
};
