#pragma once

#include "IInputParser.h"

class RawInputParser : public IInputParser {
public:
    Parameters parse(std::ifstream &aStream) override;
};
