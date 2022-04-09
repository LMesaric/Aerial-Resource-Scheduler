#include "AmplInputParser.h"
#include "RawInputParser.h"

#include <CLI11.hpp>
#include <fstream>
#include <iostream>


int main(int argc, char *argv[]) {
    const std::string myStdinFilename = "-";

    CLI::App myApp{"Aerial Resource Scheduler"};

    std::string myFilename = myStdinFilename;
    myApp.add_option(
            "-i,--input",
            myFilename,
            "Path to input file. Reads from stdin if omitted or set to a dash symbol."
    )->required(false);

    CLI11_PARSE(myApp, argc, argv);

    std::istream *myInputStream;
    std::ifstream myFile;

    if (myFilename == myStdinFilename) {
        myInputStream = &std::cin;
    } else {
        myFile.open(myFilename);

        if (myFile.fail()) {
            std::cout << "FATAL: Failed to open file " << myFilename << '\n';
            throw std::runtime_error("Failed to open file");
        }

        myInputStream = &myFile;
    }

//    auto myParser = AmplInputParser{};
    auto myParser = RawInputParser{};
    auto myParameters = myParser.parse(*myInputStream);

    return 0;
}
