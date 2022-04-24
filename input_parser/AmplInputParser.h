#pragma once

#include "IInputParser.h"

class AmplInputParser : public IInputParser {
public:
    ParametersRaw parse(std::istream &aStream) const override;
};
