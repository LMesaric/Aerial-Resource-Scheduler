#pragma once

#include "IInputParser.h"

class AmplInputParser : public IInputParser {
public:
    Parameters parse(std::ifstream &aStream) override;
};
