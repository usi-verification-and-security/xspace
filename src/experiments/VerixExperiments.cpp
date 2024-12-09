//
// Created by labbaf on 29.05.2024.
//

#include "VerixExperiments.h"
#include "Utils.h"
#include <algorithms/BasicVerix.h>
#include <verifiers/opensmt/OpenSMTVerifier.h>
#ifdef MARABOU
#include <verifiers/marabou/MarabouVerifier.h>
#endif

#include <cassert>
#include <iostream>
#include <fstream>
#include <numeric>
#include <sstream>

namespace xai::experiments {
void
VerixExperiments::experiment_on_dataset(std::string modelPath, std::string datasetPath, std::string verifier,
                                        std::string outputPath, int featureSize, float freedom_factor=0.5) {
    xai::algo::BasicVerix algo(modelPath);
    if (verifier == "OpenSMT")
        algo.setVerifier(std::make_unique<xai::verifiers::OpenSMTVerifier>());
    #ifdef MARABOU
    else if (verifier == "Marabou")
        algo.setVerifier(std::make_unique<xai::verifiers::MarabouVerifier>());
    #endif

    // Open the data CSV file
    std::ifstream file(datasetPath);
    if (!file.is_open()) {
        std::cout << "Could not open file " << datasetPath << std::endl;
        return;
    }

    //open the output CSV file
    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::cout << "Could not open file " << outputPath << std::endl;
        return;
    }

    std::vector<std::vector<float>> data;
    std::string line;
    std::string header;
    // Read the first line to skip the header

    getline(file, header);
    outputFile << "datapoint" << "," << header << '\n';
    // Iterate over each line in the file
    // TODO: Come up with heuristic for feature ordering, or random?
    std::vector<std::size_t> featureOrder(featureSize);
    if constexpr (not Config::reverseFeatures) {
        std::iota(featureOrder.begin(), featureOrder.end(), 0);
    } else {
        std::iota(featureOrder.rbegin(), featureOrder.rend(), 0);
    }
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::vector<float> row;
        std::vector<float> datapoint;
        std::cout << "row: ";
        std::cout << line << '\n';
        while (std::getline(ss, field, ',')) {
            // Convert the field to float and add it to the row vector
            row.push_back(std::stof(field));
        }
        float output = row[featureSize];
        std::cout << "-> expected output: " << output << std::endl;
        datapoint = std::vector<float>(row.begin(), row.begin() + featureSize);
        data.push_back(datapoint);


    if constexpr (Config::explanationType == Config::ExplanationType::general) {
        #ifdef MARABOU
        auto res = algo.computeGeneralizedExplanation(datapoint, featureOrder, 10); //hardcoded threshold
        #else
        auto res = algo.computeGeneralizedExplanation(datapoint, featureOrder, 3); //hardcoded threshold
        #endif
        std::cerr << "(and";
        for (auto const & constraint : res.constraints) {
            // std::cout << "Feature " << constraint.inputIndex << ' ' << constraint.opString() << ' '
            //    << constraint.value << '\n';
            std::cerr << " (" << constraint.opString() << " x" << constraint.inputIndex + 1 << " " << floatToRationalString(constraint.value) << ")";
        }
        // std::cout << '\n';
        std::cerr << ")" << std::endl << std::endl;
    } else if constexpr (Config::explanationType == Config::ExplanationType::opensmt) {
        auto res = algo.computeOpenSMTExplanation(datapoint, freedom_factor, featureOrder);
        std::cout <<"explanation: ";
        std::vector<int> explanation(featureSize, 0);
        for (auto val : res.explanation) {
            std::cout << val << " ";
            assert(val < featureSize);
            explanation.at(val) = 1;
        }
        std::cout << std::endl;

        // add results to output file
        for (int i = 0; i < datapoint.size(); ++i) {
            outputFile << datapoint[i] << " ";
        }
        outputFile << ",";
        for (int i = 0; i < explanation.size(); ++i) {
            outputFile << explanation[i];
            outputFile << ",";
        }
        outputFile << output << "," << "\n";
        std::cout << "size: " << res.explanation.size() << "/" << featureSize << std::endl;
    } else {
        auto res = algo.computeExplanation(datapoint, freedom_factor, featureOrder);
        std::cout <<"explanation: ";
        std::vector<int> explanation(featureSize, 0);
        for (auto val : res.explanation) {
            std::cout << val << " ";
            assert(val < featureSize);
            explanation.at(val) = 1;
        }
        std::cout << std::endl;

        // add results to output file
        for (int i = 0; i < datapoint.size(); ++i) {
            outputFile << datapoint[i] << " ";
        }
        outputFile << ",";
        for (int i = 0; i < explanation.size(); ++i) {
            outputFile << explanation[i];
            outputFile << ",";
        }
        outputFile << output << "," << "\n";
        std::cout << "size: " << res.explanation.size() << "/" << featureSize << std::endl;
    }
        std::cout << "#checks: " << algo.getChecksCount() << std::endl;
    }
    // Close the file
    file.close();
}
}
