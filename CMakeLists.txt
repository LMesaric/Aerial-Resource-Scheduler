cmake_minimum_required(VERSION 3.21)
project(AerialResourceScheduler)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        AerialResourceScheduler
        main.cpp
        Parameters.h
        IInputParser.h
        AmplInputParser.cpp
        AmplInputParser.h
        RawInputParser.cpp
        RawInputParser.h
)
