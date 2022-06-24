#pragma once

#include "IInputParser.h"

class RawInputParser : public IInputParser {
public:
    InstanceRaw parse(std::istream &aStream) const override;
};
