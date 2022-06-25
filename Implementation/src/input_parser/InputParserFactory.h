#pragma once

#include "AmplInputParser.h"
#include "IInputParser.h"
#include "InputFormat.h"
#include "RawInputParser.h"


class InputParserFactory {
    InputFormat theInputFormat;

public:
    explicit InputParserFactory(InputFormat aInputFormat) : theInputFormat{aInputFormat} {}

    std::unique_ptr<IInputParser> getParser() {
        switch (theInputFormat) {
            case InputFormat::Raw:
                return std::make_unique<RawInputParser>();
            default:
                return std::make_unique<AmplInputParser>();
        }
    }
};
