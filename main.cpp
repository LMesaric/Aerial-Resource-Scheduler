#include "AmplInputParser.h"
#include "RawInputParser.h"

#include <fstream>
#include <iostream>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Expected one argument: path to input file\n";
        return 1;
    }

    std::ifstream myFile{argv[1]};

    if (myFile.fail()) {
        std::cout << "FATAL: Failed to open file " << argv[1] << '\n';
        throw std::runtime_error("Failed to open file");
    }

//    auto myParser = AmplInputParser{};
    auto myParser = RawInputParser{};
    auto myParameters = myParser.parse(myFile);

    return 0;
}
