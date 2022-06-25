cmake_minimum_required(VERSION 3.21)
project(AerialResourceScheduler)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

add_executable(
        AerialResourceScheduler
        src/main.cpp
        src/input_parser/InstanceRaw.h
        src/input_parser/IInputParser.h
        src/input_parser/AmplInputParser.cpp
        src/input_parser/AmplInputParser.h
        src/input_parser/RawInputParser.cpp
        src/input_parser/RawInputParser.h
        src/input_parser/InputParserFactory.h
        src/Instance.h
        src/InstanceConverter.h
        src/Matrix.h
        src/InputFormat.h
        src/Parameters.h
        src/Schedule.h
        src/Objective.h
        src/Greedy.h
        src/Optimal.h
        src/LocalSearch.h
        src/Grasp.h
        src/Statistics.h
)
