cmake_minimum_required(VERSION 3.21)
project(AerialResourceScheduler)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

add_executable(
        AerialResourceScheduler
        main.cpp
        input_parser/ParametersRaw.h
        input_parser/IInputParser.h
        input_parser/AmplInputParser.cpp
        input_parser/AmplInputParser.h
        input_parser/RawInputParser.cpp
        input_parser/RawInputParser.h
        input_parser/InputParserFactory.h
        Parameters.h
        ParametersConverter.h
        Matrix.h)
