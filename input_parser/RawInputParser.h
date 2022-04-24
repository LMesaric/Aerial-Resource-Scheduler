#pragma once

#include "IInputParser.h"

class RawInputParser : public IInputParser {
public:
    ParametersRaw parse(std::istream &aStream) const override;
};
