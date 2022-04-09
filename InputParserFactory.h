#pragma once

#include "AmplInputParser.h"
#include "IInputParser.h"
#include "RawInputParser.h"


enum InputFormat {
    Raw,
    Ampl
};

class InputParserFactory {
    InputFormat theInputFormat;

public:
    explicit InputParserFactory(InputFormat aInputFormat) : theInputFormat{aInputFormat} {}

    std::unique_ptr<IInputParser> getParser() {
        switch (theInputFormat) {
            case InputFormat::Raw:
                return std::make_unique<RawInputParser>();
            case InputFormat::Ampl:
                return std::make_unique<AmplInputParser>();
        }
    }
};
