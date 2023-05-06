cmake_minimum_required(VERSION 3.21)
project(AerialResourceScheduler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(Threads REQUIRED)

include_directories(include)

add_executable(
        AerialResourceScheduler
        main.cpp
        input_parser/InstanceRaw.h
        input_parser/IInputParser.h
        input_parser/AmplInputParser.cpp
        input_parser/AmplInputParser.h
        input_parser/RawInputParser.cpp
        input_parser/RawInputParser.h
        input_parser/InputFormat.h
        input_parser/InputParserFactory.h
        Instance.h
        InstanceConverter.h
        Matrix.h
        Parameters.h
        Takeoff.h
        Schedule.h
        Schedule.cpp
        Objective.h
        Greedy.h
        LocalSearch.h
        Grasp.h
        Statistics.h
)

target_link_libraries(AerialResourceScheduler PRIVATE Threads::Threads)
