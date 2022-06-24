#pragma once

#include "IInputParser.h"

class AmplInputParser : public IInputParser {
public:
    InstanceRaw parse(std::istream &aStream) const override;
};
