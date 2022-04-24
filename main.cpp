#include "input_parser/InputParserFactory.h"
#include "ParametersConverter.h"

#include <CLI11.hpp>
#include <iostream>
#include <map>


int main(int argc, char *argv[]) {
    const std::string myStdinFilename = "-";

    CLI::App myApp{"Aerial Resource Scheduler"};

    std::string myFilename{myStdinFilename};
    myApp.add_option(
            "-i,--input",
            myFilename,
            "Path to input file. Reads from stdin if omitted or set to a dash symbol."
    )->required(false);

    InputFormat myInputFormat{};
    std::map<std::string, InputFormat> myInputFormatMap{
            {"raw",  InputFormat::Raw},
            {"ampl", InputFormat::Ampl}};
    myApp.add_option("-f,--format", myInputFormat, "Input data format.")
            ->required(true)
            ->transform(CLI::CheckedTransformer(myInputFormatMap, CLI::ignore_case));

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

    const auto myParser = InputParserFactory(myInputFormat).getParser();
    const auto myParametersRaw = myParser->parse(*myInputStream);
    const auto myParameters = convertParameters(myParametersRaw);

    return 0;
}
