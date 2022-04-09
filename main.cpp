#include "AmplInputParser.h"
#include "RawInputParser.h"

#include <CLI11.hpp>
#include <fstream>
#include <iostream>


int main(int argc, char *argv[]) {
    CLI::App myApp{"Aerial Resource Scheduler"};

    std::string myFilename;
    myApp.add_option("-i,--input", myFilename, "Path to input file")->required();

    CLI11_PARSE(myApp, argc, argv);

    std::ifstream myFile{myFilename};

    if (myFile.fail()) {
        std::cout << "FATAL: Failed to open file " << myFilename << '\n';
        throw std::runtime_error("Failed to open file");
    }

//    auto myParser = AmplInputParser{};
    auto myParser = RawInputParser{};
    auto myParameters = myParser.parse(myFile);

    return 0;
}
